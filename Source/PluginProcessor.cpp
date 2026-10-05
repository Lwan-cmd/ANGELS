#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

namespace
{
    constexpr float twoPi = juce::MathConstants<float>::twoPi;

    float onePoleCoefficient (float cutoff, double sampleRate)
    {
        cutoff = juce::jlimit (20.0f, static_cast<float> (sampleRate * 0.45), cutoff);
        return 1.0f - std::exp (-twoPi * cutoff / static_cast<float> (sampleRate));
    }
}

AngelEngineAudioProcessor::AngelEngineAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout AngelEngineAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto addMacro = [&layout] (const char* id, const char* name, float defaultValue)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 },
            name,
            juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
            defaultValue));
    };

    addMacro ("age",   "AGE",   0.18f);
    addMacro ("air",   "AIR",   0.22f);
    addMacro ("ghost", "GHOST", 0.12f);
    addMacro ("width", "WIDTH", 0.25f);
    addMacro ("melt",  "MELT",  0.30f);
    addMacro ("chaos", "CHAOS", 0.08f);
    addMacro ("grain", "GRAIN", 0.00f);
    addMacro ("mix",   "MIX",   1.00f);

    const juce::StringArray shapes { "SINE", "TRIANGLE", "SAW", "SQUARE", "RANDOM" };
    const juce::StringArray destinations {
        "NONE", "AGE", "AIR", "GHOST", "WIDTH", "MELT", "CHAOS", "GRAIN"
    };

    for (int i = 1; i <= 3; ++i)
    {
        const auto n = juce::String (i);

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "lfo" + n + "Rate", 1 },
            "LFO " + n + " RATE",
            juce::NormalisableRange<float> { 0.03f, 8.0f, 0.001f, 0.38f },
            0.25f * static_cast<float> (i)));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "lfo" + n + "Depth", 1 },
            "LFO " + n + " DEPTH",
            juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
            0.0f));

        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "lfo" + n + "Shape", 1 },
            "LFO " + n + " SHAPE",
            shapes,
            i == 3 ? 4 : 0));

        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "lfo" + n + "Dest", 1 },
            "LFO " + n + " DESTINATION",
            destinations,
            0));
    }

    return layout;
}

void AngelEngineAudioProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;

    const auto meltSize = static_cast<int> (std::ceil (sr * 0.060)) + 4;
    const auto ghostSize = static_cast<int> (std::ceil (sr * 2.0)) + 4;
    const auto grainSize = static_cast<int> (std::ceil (sr * 2.5)) + 4;

    meltDelay.assign (static_cast<size_t> (meltSize), { 0.0f, 0.0f });
    ghostDelay.assign (static_cast<size_t> (ghostSize), { 0.0f, 0.0f });
    grainBuffer.assign (static_cast<size_t> (grainSize), { 0.0f, 0.0f });

    meltWrite = 0;
    ghostWrite = 0;
    grainWrite = 0;
    heldSample = { 0.0f, 0.0f };
    holdCounter = { 0, 0 };
    ageLowpassState = { 0.0f, 0.0f };
    airLowpassState = { 0.0f, 0.0f };
    meltPhase = 0.0;
    chaosPhase = 0.0;
    lfoPhase = { 0.0, 0.0, 0.0 };
    grains = {};
    samplesUntilNextGrain = 0;
}

bool AngelEngineAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& input = layouts.getMainInputChannelSet();
    const auto& output = layouts.getMainOutputChannelSet();

    return input == juce::AudioChannelSet::stereo()
        && output == juce::AudioChannelSet::stereo();
}

float AngelEngineAudioProcessor::readInterpolated (
    const std::vector<std::array<float, 2>>& buffer,
    int writePosition,
    float delaySamples,
    int channel) const
{
    if (buffer.empty())
        return 0.0f;

    const auto size = static_cast<int> (buffer.size());
    float readPosition = static_cast<float> (writePosition) - delaySamples;

    while (readPosition < 0.0f)
        readPosition += static_cast<float> (size);

    const int i0 = static_cast<int> (readPosition) % size;
    const int i1 = (i0 + 1) % size;
    const float frac = readPosition - std::floor (readPosition);

    const float a = buffer[static_cast<size_t> (i0)][static_cast<size_t> (channel)];
    const float b = buffer[static_cast<size_t> (i1)][static_cast<size_t> (channel)];

    return juce::jmap (frac, a, b);
}

float AngelEngineAudioProcessor::readCircularAt (double position, int channel) const
{
    if (grainBuffer.empty())
        return 0.0f;

    const auto size = static_cast<int> (grainBuffer.size());

    while (position < 0.0)
        position += static_cast<double> (size);
    while (position >= static_cast<double> (size))
        position -= static_cast<double> (size);

    const int i0 = static_cast<int> (std::floor (position));
    const int i1 = (i0 + 1) % size;
    const float frac = static_cast<float> (position - std::floor (position));

    const float a = grainBuffer[static_cast<size_t> (i0)][static_cast<size_t> (channel)];
    const float b = grainBuffer[static_cast<size_t> (i1)][static_cast<size_t> (channel)];
    return a + (b - a) * frac;
}

float AngelEngineAudioProcessor::nextGrainRandom()
{
    grainRandomState = grainRandomState * 1664525u + 1013904223u;
    return static_cast<float> (grainRandomState & 0x00ffffffu)
         / static_cast<float> (0x00ffffffu);
}

void AngelEngineAudioProcessor::spawnGrain (float amount)
{
    if (grainBuffer.empty() || amount <= 0.0f)
        return;

    GrainVoice* voice = nullptr;

    for (auto& candidate : grains)
    {
        if (! candidate.active)
        {
            voice = &candidate;
            break;
        }
    }

    if (voice == nullptr)
    {
        voice = &*std::max_element (
            grains.begin(), grains.end(),
            [] (const GrainVoice& a, const GrainVoice& b)
            {
                return a.ageSamples < b.ageSamples;
            });
    }

    const float r1 = nextGrainRandom();
    const float r2 = nextGrainRandom();
    const float r3 = nextGrainRandom();
    const float r4 = nextGrainRandom();
    const float r5 = nextGrainRandom();

    const float maxLookbackMs = 120.0f + amount * 780.0f;
    const float lookbackMs = 35.0f + r1 * maxLookbackMs;
    const float delaySamples = lookbackMs * static_cast<float> (sr) / 1000.0f;

    const float minLengthMs = 24.0f + (1.0f - amount) * 52.0f;
    const float maxLengthMs = 72.0f + (1.0f - amount) * 110.0f;
    const float lengthMs = minLengthMs + r2 * (maxLengthMs - minLengthMs);

    const float pitchSpread = 1.0f + amount * 11.0f;
    float semitones = std::round ((r3 * 2.0f - 1.0f) * pitchSpread);
    if (amount < 0.12f)
        semitones = 0.0f;

    const bool reverse = r4 < (0.04f + amount * amount * 0.34f);
    const double ratio = std::pow (2.0, static_cast<double> (semitones) / 12.0);

    voice->active = true;
    voice->readPosition = static_cast<double> (grainWrite) - static_cast<double> (delaySamples);
    voice->increment = reverse ? -ratio : ratio;
    voice->ageSamples = 0;
    voice->lengthSamples = juce::jmax (8, static_cast<int> (lengthMs * static_cast<float> (sr) / 1000.0f));
    voice->pan = (r5 * 2.0f - 1.0f) * juce::jmin (0.75f, amount * 0.75f);
    voice->gain = 0.72f + (1.0f - amount) * 0.16f;
}

float AngelEngineAudioProcessor::getLfoValue (int lfoIndex, int shape) const
{
    const float norm = static_cast<float> (
        lfoPhase[static_cast<size_t> (lfoIndex)] / juce::MathConstants<double>::twoPi);

    switch (shape)
    {
        case 1: // triangle
            if (norm < 0.25f) return norm * 4.0f;
            if (norm < 0.75f) return 2.0f - norm * 4.0f;
            return norm * 4.0f - 4.0f;

        case 2: // saw
            return norm * 2.0f - 1.0f;

        case 3: // square
            return norm < 0.5f ? 1.0f : -1.0f;

        case 4: // sample & hold
            return randomHeld[static_cast<size_t> (lfoIndex)];

        default:
            return std::sin (static_cast<float> (lfoPhase[static_cast<size_t> (lfoIndex)]));
    }
}

void AngelEngineAudioProcessor::advanceLfo (int lfoIndex, float rateHz)
{
    auto& phase = lfoPhase[static_cast<size_t> (lfoIndex)];
    phase += juce::MathConstants<double>::twoPi * static_cast<double> (rateHz) / sr;

    if (phase >= juce::MathConstants<double>::twoPi)
    {
        phase -= juce::MathConstants<double>::twoPi;

        auto& state = randomState[static_cast<size_t> (lfoIndex)];
        state = state * 1664525u + 1013904223u;
        const float unit = static_cast<float> (state & 0x00ffffffu)
                         / static_cast<float> (0x00ffffffu);
        randomHeld[static_cast<size_t> (lfoIndex)] = unit * 2.0f - 1.0f;
    }
}

void AngelEngineAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    if (buffer.getNumChannels() < 2
        || meltDelay.empty()
        || ghostDelay.empty()
        || grainBuffer.empty())
        return;

    // Macro order here matches the LFO destination list, not the visual order.
    const std::array<float, 7> base {
        apvts.getRawParameterValue ("age")->load(),
        apvts.getRawParameterValue ("air")->load(),
        apvts.getRawParameterValue ("ghost")->load(),
        apvts.getRawParameterValue ("width")->load(),
        apvts.getRawParameterValue ("melt")->load(),
        apvts.getRawParameterValue ("chaos")->load(),
        apvts.getRawParameterValue ("grain")->load()
    };

    const float mix = apvts.getRawParameterValue ("mix")->load();

    std::array<float, 3> lfoRates {};
    std::array<float, 3> lfoDepths {};
    std::array<int, 3> lfoShapes {};
    std::array<int, 3> lfoDests {};

    for (int lfo = 0; lfo < 3; ++lfo)
    {
        const auto n = juce::String (lfo + 1);
        lfoRates[static_cast<size_t> (lfo)] =
            apvts.getRawParameterValue ("lfo" + n + "Rate")->load();
        lfoDepths[static_cast<size_t> (lfo)] =
            apvts.getRawParameterValue ("lfo" + n + "Depth")->load();
        lfoShapes[static_cast<size_t> (lfo)] = static_cast<int> (std::round (
            apvts.getRawParameterValue ("lfo" + n + "Shape")->load()));
        lfoDests[static_cast<size_t> (lfo)] = static_cast<int> (std::round (
            apvts.getRawParameterValue ("lfo" + n + "Dest")->load()));
    }

    auto* left = buffer.getWritePointer (0);
    auto* right = buffer.getWritePointer (1);
    const int numSamples = buffer.getNumSamples();

    for (int i = 0; i < numSamples; ++i)
    {
        std::array<float, 7> modulated = base;

        for (int lfo = 0; lfo < 3; ++lfo)
        {
            const float lfoValue = getLfoValue (lfo, lfoShapes[static_cast<size_t> (lfo)]);
            const int dest = lfoDests[static_cast<size_t> (lfo)];

            if (dest >= 1 && dest <= 7)
            {
                auto& target = modulated[static_cast<size_t> (dest - 1)];
                target = juce::jlimit (
                    0.0f, 1.0f,
                    target + lfoValue * lfoDepths[static_cast<size_t> (lfo)] * 0.50f);
            }

            advanceLfo (lfo, lfoRates[static_cast<size_t> (lfo)]);
        }

        const float age   = modulated[0];
        const float air   = modulated[1];
        const float ghost = modulated[2];
        const float width = modulated[3];
        const float melt  = modulated[4];
        const float chaos = modulated[5];
        const float grain = modulated[6];

        const float dryL = left[i];
        const float dryR = right[i];

        const int holdPeriod = 1 + static_cast<int> (std::round (age * age * 12.0f));
        const float ageMix = age * 0.70f;
        const float ageLPAlpha = onePoleCoefficient (juce::jmap (age, 18000.0f, 3500.0f), sr);
        const float airLPAlpha = onePoleCoefficient (4200.0f, sr);
        const float airAmount = air * 0.45f;

        const float ghostDelayMs = juce::jmap (ghost, 180.0f, 520.0f);
        const float ghostDelaySamples = ghostDelayMs * static_cast<float> (sr) / 1000.0f;
        const float ghostFeedbackBase = juce::jmap (ghost, 0.18f, 0.58f);
        const float ghostWet = ghost * 0.50f;

        const float sideGain = 1.0f + width * 1.25f;
        const float widthCompensation = 1.0f / (1.0f + width * 0.22f);

        const float chaosWave = std::sin (static_cast<float> (chaosPhase))
                              * std::sin (static_cast<float> (chaosPhase * 0.371 + 1.1));

        chaosPhase += twoPi * (0.071 + chaos * 0.16) / sr;
        if (chaosPhase >= twoPi)
            chaosPhase -= twoPi;

        const float meltRate = 0.13f + chaos * 0.42f;
        const float baseDelayMs = 2.2f + melt * 8.0f;
        const float modDepthMs = melt * (1.2f + chaos * 5.0f);
        const float meltLfo = std::sin (static_cast<float> (meltPhase));
        const float modulatedDelayMs = juce::jlimit (
            0.7f, 45.0f,
            baseDelayMs + modDepthMs * (0.72f * meltLfo + 0.28f * chaosWave));

        meltPhase += twoPi * meltRate / sr;
        if (meltPhase >= twoPi)
            meltPhase -= twoPi;

        const std::array<float, 2> input { dryL, dryR };
        std::array<float, 2> preGrain {};

        // AGE and MELT happen before the granular memory, so the grain buffer
        // captures the worn / unstable version of the source rather than raw audio.
        for (int ch = 0; ch < 2; ++ch)
        {
            float x = input[static_cast<size_t> (ch)];

            if (holdCounter[static_cast<size_t> (ch)] <= 0)
            {
                heldSample[static_cast<size_t> (ch)] = x;
                holdCounter[static_cast<size_t> (ch)] = holdPeriod;
            }

            --holdCounter[static_cast<size_t> (ch)];

            const float crushed = heldSample[static_cast<size_t> (ch)];
            x = juce::jmap (ageMix, x, crushed);

            ageLowpassState[static_cast<size_t> (ch)] +=
                ageLPAlpha * (x - ageLowpassState[static_cast<size_t> (ch)]);

            x = juce::jmap (age * 0.65f, x, ageLowpassState[static_cast<size_t> (ch)]);

            meltDelay[static_cast<size_t> (meltWrite)][static_cast<size_t> (ch)] = x;
            const float meltDelaySamples = modulatedDelayMs * static_cast<float> (sr) / 1000.0f;
            const float melted = readInterpolated (meltDelay, meltWrite, meltDelaySamples, ch);

            x = juce::jmap (melt * 0.68f, x, melted);

            const float drive = 1.0f + melt * 2.8f + chaos * 0.45f;
            x = std::tanh (x * drive) / std::tanh (drive);
            preGrain[static_cast<size_t> (ch)] = x;
        }

        grainBuffer[static_cast<size_t> (grainWrite)] = preGrain;

        if (grain > 0.01f)
        {
            if (samplesUntilNextGrain <= 0)
            {
                spawnGrain (grain);
                const float densityHz = 2.5f + grain * 31.5f;
                samplesUntilNextGrain = juce::jmax (
                    1, static_cast<int> (static_cast<float> (sr) / densityHz));
            }
            else
            {
                --samplesUntilNextGrain;
            }
        }
        else
        {
            samplesUntilNextGrain = 0;
        }

        std::array<float, 2> grainAccum { 0.0f, 0.0f };
        int activeGrains = 0;

        for (auto& voice : grains)
        {
            if (! voice.active)
                continue;

            const int windowDenominator = juce::jmax (1, voice.lengthSamples - 1);
            const float grainPhase = static_cast<float> (voice.ageSamples)
                                   / static_cast<float> (windowDenominator);
            const float window = 0.5f - 0.5f * std::cos (twoPi * grainPhase);

            const float leftGain = 1.0f - juce::jmax (0.0f, voice.pan) * 0.68f;
            const float rightGain = 1.0f + juce::jmin (0.0f, voice.pan) * 0.68f;

            grainAccum[0] += readCircularAt (voice.readPosition, 0)
                           * window * voice.gain * leftGain;
            grainAccum[1] += readCircularAt (voice.readPosition, 1)
                           * window * voice.gain * rightGain;

            voice.readPosition += voice.increment;

            const double grainSize = static_cast<double> (grainBuffer.size());
            while (voice.readPosition < 0.0)
                voice.readPosition += grainSize;
            while (voice.readPosition >= grainSize)
                voice.readPosition -= grainSize;

            ++voice.ageSamples;
            ++activeGrains;

            if (voice.ageSamples >= voice.lengthSamples)
                voice.active = false;
        }

        std::array<float, 2> granular = preGrain;
        if (activeGrains > 0)
        {
            const float normalise = 1.0f / std::sqrt (static_cast<float> (activeGrains));
            granular[0] = grainAccum[0] * normalise;
            granular[1] = grainAccum[1] * normalise;
        }

        const float grainWet = grain * 0.82f;
        std::array<float, 2> processed {
            preGrain[0] + (granular[0] - preGrain[0]) * grainWet,
            preGrain[1] + (granular[1] - preGrain[1]) * grainWet
        };

        // AIR and GHOST operate after the granular section, helping the grains
        // live in the same spectral and spatial world as the direct signal.
        for (int ch = 0; ch < 2; ++ch)
        {
            float x = processed[static_cast<size_t> (ch)];

            airLowpassState[static_cast<size_t> (ch)] +=
                airLPAlpha * (x - airLowpassState[static_cast<size_t> (ch)]);

            const float high = x - airLowpassState[static_cast<size_t> (ch)];
            x += high * airAmount;

            const float ghostRead = readInterpolated (ghostDelay, ghostWrite, ghostDelaySamples, ch);
            const float feedback = juce::jlimit (
                0.0f, 0.74f,
                ghostFeedbackBase + chaos * 0.08f * chaosWave);

            ghostDelay[static_cast<size_t> (ghostWrite)][static_cast<size_t> (ch)] =
                x + ghostRead * feedback;

            x += ghostRead * ghostWet;
            processed[static_cast<size_t> (ch)] = x;
        }

        const float mid = 0.5f * (processed[0] + processed[1]);
        const float side = 0.5f * (processed[0] - processed[1]) * sideGain;

        const float wetL = std::tanh ((mid + side) * widthCompensation);
        const float wetR = std::tanh ((mid - side) * widthCompensation);

        left[i]  = dryL + (wetL - dryL) * mix;
        right[i] = dryR + (wetR - dryR) * mix;

        meltWrite = (meltWrite + 1) % static_cast<int> (meltDelay.size());
        ghostWrite = (ghostWrite + 1) % static_cast<int> (ghostDelay.size());
        grainWrite = (grainWrite + 1) % static_cast<int> (grainBuffer.size());
    }
}

void AngelEngineAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void AngelEngineAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* AngelEngineAudioProcessor::createEditor()
{
    return new AngelEngineAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AngelEngineAudioProcessor();
}
