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

IngeniumAudioProcessor::IngeniumAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout IngeniumAudioProcessor::createParameterLayout()
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

    addMacro ("age",   "AGE",   0.16f);
    addMacro ("melt",  "MELT",  0.24f);
    addMacro ("grain", "GRAIN", 0.00f);
    addMacro ("ghost", "GHOST", 0.10f);
    addMacro ("smear", "SMEAR", 0.00f);
    addMacro ("chaos", "CHAOS", 0.06f);

    addMacro ("air",   "AIR",   0.20f);
    addMacro ("width", "WIDTH", 0.22f);
    addMacro ("mix",   "MIX",   1.00f);

    const juce::StringArray shapes {
        "SINE", "TRIANGLE", "SAW", "SQUARE", "RANDOM"
    };

    const juce::StringArray coreModules {
        "NONE", "AGE", "MELT", "GRAIN", "GHOST", "SMEAR", "CHAOS"
    };

    for (int i = 1; i <= 3; ++i)
    {
        const auto n = juce::String (i);

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "lfo" + n + "Rate", 1 },
            "LFO " + n + " RATE",
            juce::NormalisableRange<float> { 0.03f, 8.0f, 0.001f, 0.38f },
            0.22f * static_cast<float> (i)));

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
            coreModules,
            0));
    }

    const juce::StringArray matrixModes { "CONTROL", "AUDIO" };

    for (int i = 1; i <= 4; ++i)
    {
        const auto n = juce::String (i);

        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "matrix" + n + "Source", 1 },
            "MATRIX " + n + " SOURCE",
            coreModules,
            0));

        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "matrix" + n + "Dest", 1 },
            "MATRIX " + n + " TARGET",
            coreModules,
            0));

        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "matrix" + n + "Mode", 1 },
            "MATRIX " + n + " MODE",
            matrixModes,
            0));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "matrix" + n + "Amount", 1 },
            "MATRIX " + n + " AMOUNT",
            juce::NormalisableRange<float> { -1.0f, 1.0f, 0.001f },
            0.0f));
    }

    return layout;
}

void IngeniumAudioProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;

    meltDelay.assign (
        static_cast<size_t> (std::ceil (sr * 0.070)) + 4,
        { 0.0f, 0.0f });

    ghostDelay.assign (
        static_cast<size_t> (std::ceil (sr * 2.5)) + 4,
        { 0.0f, 0.0f });

    grainBuffer.assign (
        static_cast<size_t> (std::ceil (sr * 2.5)) + 4,
        { 0.0f, 0.0f });

    smearBuffer.assign (
        static_cast<size_t> (std::ceil (sr * 0.180)) + 4,
        { 0.0f, 0.0f });

    meltWrite = 0;
    ghostWrite = 0;
    grainWrite = 0;
    smearWrite = 0;

    heldSample = { 0.0f, 0.0f };
    holdCounter = { 0, 0 };
    ageLowpassState = { 0.0f, 0.0f };
    airLowpassState = { 0.0f, 0.0f };
    smearDampState = { 0.0f, 0.0f };

    meltPhase = 0.0;
    chaosPhase = 0.0;
    lfoPhase = { 0.0, 0.0, 0.0 };

    matrixSourceTaps = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    matrixControlState = { 0.0f, 0.0f, 0.0f, 0.0f };

    grains = {};
    samplesUntilNextGrain = 0;
}

bool IngeniumAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

float IngeniumAudioProcessor::readInterpolated (
    const std::vector<std::array<float, 2>>& buffer,
    int writePosition,
    float delaySamples,
    int channel) const
{
    if (buffer.empty())
        return 0.0f;

    const int size = static_cast<int> (buffer.size());
    float readPosition = static_cast<float> (writePosition) - delaySamples;

    while (readPosition < 0.0f)
        readPosition += static_cast<float> (size);

    while (readPosition >= static_cast<float> (size))
        readPosition -= static_cast<float> (size);

    const int i0 = static_cast<int> (std::floor (readPosition)) % size;
    const int i1 = (i0 + 1) % size;
    const float frac = readPosition - std::floor (readPosition);

    const float a = buffer[static_cast<size_t> (i0)][static_cast<size_t> (channel)];
    const float b = buffer[static_cast<size_t> (i1)][static_cast<size_t> (channel)];
    return a + (b - a) * frac;
}

float IngeniumAudioProcessor::readGrainAt (double position, int channel) const
{
    if (grainBuffer.empty())
        return 0.0f;

    const int size = static_cast<int> (grainBuffer.size());

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

float IngeniumAudioProcessor::nextGrainRandom()
{
    grainRandomState = grainRandomState * 1664525u + 1013904223u;
    return static_cast<float> (grainRandomState & 0x00ffffffu)
         / static_cast<float> (0x00ffffffu);
}

void IngeniumAudioProcessor::spawnGrain (float amount, float chaos)
{
    if (grainBuffer.empty() || amount <= 0.001f)
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

    const float maxLookbackMs = 100.0f + amount * 850.0f;
    const float lookbackMs = 25.0f + r1 * maxLookbackMs;
    const float delaySamples = lookbackMs * static_cast<float> (sr) / 1000.0f;

    const float minLengthMs = 20.0f + (1.0f - amount) * 55.0f;
    const float maxLengthMs = 65.0f + (1.0f - amount) * 130.0f;
    const float lengthMs = minLengthMs + r2 * (maxLengthMs - minLengthMs);

    const float pitchSpread = 1.0f + amount * 11.0f + chaos * 2.0f;
    float semitones = std::round ((r3 * 2.0f - 1.0f) * pitchSpread);

    if (amount < 0.10f)
        semitones = 0.0f;

    const bool reverse = r4 < (0.03f + amount * amount * 0.36f + chaos * 0.05f);
    const double ratio = std::pow (2.0, static_cast<double> (semitones) / 12.0);

    voice->active = true;
    voice->readPosition = static_cast<double> (grainWrite) - static_cast<double> (delaySamples);
    voice->increment = reverse ? -ratio : ratio;
    voice->ageSamples = 0;
    voice->lengthSamples = juce::jmax (
        16,
        static_cast<int> (lengthMs * static_cast<float> (sr) / 1000.0f));
    voice->pan = (r5 * 2.0f - 1.0f) * juce::jmin (0.85f, amount * 0.85f);
    voice->gain = 0.65f + (1.0f - amount) * 0.20f;
}

float IngeniumAudioProcessor::getLfoValue (int lfoIndex, int shape) const
{
    const float norm = static_cast<float> (
        lfoPhase[static_cast<size_t> (lfoIndex)]
        / juce::MathConstants<double>::twoPi);

    switch (shape)
    {
        case 1:
            if (norm < 0.25f) return norm * 4.0f;
            if (norm < 0.75f) return 2.0f - norm * 4.0f;
            return norm * 4.0f - 4.0f;

        case 2:
            return norm * 2.0f - 1.0f;

        case 3:
            return norm < 0.5f ? 1.0f : -1.0f;

        case 4:
            return randomHeld[static_cast<size_t> (lfoIndex)];

        default:
            return std::sin (
                static_cast<float> (lfoPhase[static_cast<size_t> (lfoIndex)]));
    }
}

void IngeniumAudioProcessor::advanceLfo (int lfoIndex, float rateHz)
{
    auto& phase = lfoPhase[static_cast<size_t> (lfoIndex)];

    phase += juce::MathConstants<double>::twoPi
           * static_cast<double> (rateHz) / sr;

    if (phase >= juce::MathConstants<double>::twoPi)
    {
        phase -= juce::MathConstants<double>::twoPi;

        auto& state = randomState[static_cast<size_t> (lfoIndex)];
        state = state * 1664525u + 1013904223u;

        const float unit =
            static_cast<float> (state & 0x00ffffffu)
            / static_cast<float> (0x00ffffffu);

        randomHeld[static_cast<size_t> (lfoIndex)] = unit * 2.0f - 1.0f;
    }
}

float IngeniumAudioProcessor::monoTap (const std::array<float, 2>& stereo)
{
    return std::tanh (0.75f * (stereo[0] + stereo[1]));
}

void IngeniumAudioProcessor::processBlock (
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    if (buffer.getNumChannels() < 2
        || meltDelay.empty()
        || ghostDelay.empty()
        || grainBuffer.empty()
        || smearBuffer.empty())
        return;

    const std::array<float, 6> base {
        apvts.getRawParameterValue ("age")->load(),
        apvts.getRawParameterValue ("melt")->load(),
        apvts.getRawParameterValue ("grain")->load(),
        apvts.getRawParameterValue ("ghost")->load(),
        apvts.getRawParameterValue ("smear")->load(),
        apvts.getRawParameterValue ("chaos")->load()
    };

    const float air = apvts.getRawParameterValue ("air")->load();
    const float width = apvts.getRawParameterValue ("width")->load();
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

        lfoShapes[static_cast<size_t> (lfo)] = static_cast<int> (
            std::round (apvts.getRawParameterValue ("lfo" + n + "Shape")->load()));

        lfoDests[static_cast<size_t> (lfo)] = static_cast<int> (
            std::round (apvts.getRawParameterValue ("lfo" + n + "Dest")->load()));
    }

    std::array<int, 4> matrixSources {};
    std::array<int, 4> matrixDests {};
    std::array<int, 4> matrixModes {};
    std::array<float, 4> matrixAmounts {};

    for (int route = 0; route < 4; ++route)
    {
        const auto n = juce::String (route + 1);

        matrixSources[static_cast<size_t> (route)] = static_cast<int> (
            std::round (apvts.getRawParameterValue ("matrix" + n + "Source")->load()));

        matrixDests[static_cast<size_t> (route)] = static_cast<int> (
            std::round (apvts.getRawParameterValue ("matrix" + n + "Dest")->load()));

        matrixModes[static_cast<size_t> (route)] = static_cast<int> (
            std::round (apvts.getRawParameterValue ("matrix" + n + "Mode")->load()));

        matrixAmounts[static_cast<size_t> (route)] =
            apvts.getRawParameterValue ("matrix" + n + "Amount")->load();
    }

    const float controlAlpha = onePoleCoefficient (14.0f, sr);

    auto* left = buffer.getWritePointer (0);
    auto* right = buffer.getWritePointer (1);
    const int numSamples = buffer.getNumSamples();

    for (int i = 0; i < numSamples; ++i)
    {
        std::array<float, 6> modulated = base;

        for (int lfo = 0; lfo < 3; ++lfo)
        {
            const float value =
                getLfoValue (lfo, lfoShapes[static_cast<size_t> (lfo)]);

            const int dest = lfoDests[static_cast<size_t> (lfo)];

            if (dest >= 1 && dest <= 6)
            {
                auto& target = modulated[static_cast<size_t> (dest - 1)];

                target = juce::jlimit (
                    0.0f,
                    1.0f,
                    target + value
                        * lfoDepths[static_cast<size_t> (lfo)]
                        * 0.50f);
            }

            advanceLfo (lfo, lfoRates[static_cast<size_t> (lfo)]);
        }

        for (int route = 0; route < 4; ++route)
        {
            const int source = matrixSources[static_cast<size_t> (route)];
            const int dest = matrixDests[static_cast<size_t> (route)];

            if (source < 1 || source > 6 || dest < 1 || dest > 6)
                continue;

            const float raw =
                matrixSourceTaps[static_cast<size_t> (source - 1)];

            auto& control =
                matrixControlState[static_cast<size_t> (route)];

            control += controlAlpha * (raw - control);

            const bool audioRate =
                matrixModes[static_cast<size_t> (route)] == 1;

            const float modSignal =
                audioRate ? raw : control;

            const float range =
                audioRate ? 0.24f : 0.48f;

            auto& target =
                modulated[static_cast<size_t> (dest - 1)];

            target = juce::jlimit (
                0.0f,
                1.0f,
                target + modSignal
                    * matrixAmounts[static_cast<size_t> (route)]
                    * range);
        }

        const float age   = modulated[0];
        const float melt  = modulated[1];
        const float grain = modulated[2];
        const float ghost = modulated[3];
        const float smear = modulated[4];
        const float chaos = modulated[5];

        const float dryL = left[i];
        const float dryR = right[i];

        const float chaosWave =
            std::sin (static_cast<float> (chaosPhase))
            * std::sin (static_cast<float> (chaosPhase * 0.371 + 1.1));

        chaosPhase += twoPi * (0.055 + chaos * 0.23) / sr;

        if (chaosPhase >= twoPi)
            chaosPhase -= twoPi;

        matrixSourceTaps[5] = chaosWave;

        const int holdPeriod =
            1 + static_cast<int> (std::round (age * age * 14.0f));

        const float ageMix = age * 0.72f;
        const float ageCutoff = juce::jmap (age, 19000.0f, 3000.0f);
        const float ageAlpha = onePoleCoefficient (ageCutoff, sr);

        const float meltRate = 0.10f + melt * 0.23f + chaos * 0.40f;
        const float meltBaseMs = 2.0f + melt * 8.5f;
        const float meltDepthMs = melt * (1.2f + chaos * 5.8f);
        const float meltWave = std::sin (static_cast<float> (meltPhase));

        const float meltDelayMs = juce::jlimit (
            0.6f,
            48.0f,
            meltBaseMs
                + meltDepthMs * (0.74f * meltWave + 0.26f * chaosWave));

        meltPhase += twoPi * meltRate / sr;

        if (meltPhase >= twoPi)
            meltPhase -= twoPi;

        std::array<float, 2> stage { dryL, dryR };

        for (int ch = 0; ch < 2; ++ch)
        {
            float x = stage[static_cast<size_t> (ch)];

            if (holdCounter[static_cast<size_t> (ch)] <= 0)
            {
                heldSample[static_cast<size_t> (ch)] = x;
                holdCounter[static_cast<size_t> (ch)] = holdPeriod;
            }

            --holdCounter[static_cast<size_t> (ch)];

            x += (heldSample[static_cast<size_t> (ch)] - x) * ageMix;

            ageLowpassState[static_cast<size_t> (ch)] +=
                ageAlpha
                * (x - ageLowpassState[static_cast<size_t> (ch)]);

            x += (ageLowpassState[static_cast<size_t> (ch)] - x)
               * (age * 0.68f);

            stage[static_cast<size_t> (ch)] = x;
        }

        matrixSourceTaps[0] = monoTap (stage);

        for (int ch = 0; ch < 2; ++ch)
        {
            const float x = stage[static_cast<size_t> (ch)];

            meltDelay[static_cast<size_t> (meltWrite)]
                     [static_cast<size_t> (ch)] = x;

            const float delayed = readInterpolated (
                meltDelay,
                meltWrite,
                meltDelayMs * static_cast<float> (sr) / 1000.0f,
                ch);

            float y = x + (delayed - x) * (melt * 0.68f);

            const float drive = 1.0f + melt * 2.6f + chaos * 0.38f;
            y = std::tanh (y * drive) / std::tanh (drive);

            stage[static_cast<size_t> (ch)] = y;
        }

        matrixSourceTaps[1] = monoTap (stage);

        grainBuffer[static_cast<size_t> (grainWrite)] = stage;

        if (grain > 0.001f)
        {
            --samplesUntilNextGrain;

            if (samplesUntilNextGrain <= 0)
            {
                spawnGrain (grain, chaos);

                const float density =
                    2.0f + grain * 32.0f + chaos * 3.0f;

                samplesUntilNextGrain = juce::jmax (
                    1,
                    static_cast<int> (sr / static_cast<double> (density)));
            }
        }
        else
        {
            samplesUntilNextGrain = 0;
        }

        std::array<float, 2> grainSum { 0.0f, 0.0f };
        int activeGrains = 0;

        for (auto& voice : grains)
        {
            if (! voice.active)
                continue;

            const float position =
                static_cast<float> (voice.ageSamples)
                / static_cast<float> (juce::jmax (1, voice.lengthSamples));

            if (position >= 1.0f)
            {
                voice.active = false;
                continue;
            }

            const float env =
                0.5f - 0.5f * std::cos (twoPi * position);

            const float panL = std::sqrt (
                0.5f * juce::jlimit (0.0f, 2.0f, 1.0f - voice.pan));

            const float panR = std::sqrt (
                0.5f * juce::jlimit (0.0f, 2.0f, 1.0f + voice.pan));

            grainSum[0] += readGrainAt (voice.readPosition, 0)
                         * env * voice.gain * panL;

            grainSum[1] += readGrainAt (voice.readPosition, 1)
                         * env * voice.gain * panR;

            voice.readPosition += voice.increment;
            ++voice.ageSamples;
            ++activeGrains;
        }

        if (activeGrains > 0)
        {
            const float norm =
                1.0f / std::sqrt (static_cast<float> (activeGrains));

            const float grainWet = grain * 0.72f;

            stage[0] += (grainSum[0] * norm - stage[0]) * grainWet;
            stage[1] += (grainSum[1] * norm - stage[1]) * grainWet;
        }

        matrixSourceTaps[2] = monoTap (stage);

        for (int ch = 0; ch < 2; ++ch)
        {
            const float x = stage[static_cast<size_t> (ch)];

            const float d1 = (4.5f + smear * 7.0f)
                           * static_cast<float> (sr) / 1000.0f;

            const float d2 = (11.0f + smear * 17.0f)
                           * static_cast<float> (sr) / 1000.0f;

            const float d3 = (24.0f + smear * 31.0f)
                           * static_cast<float> (sr) / 1000.0f;

            const float tap1 = readInterpolated (
                smearBuffer, smearWrite, d1, ch);

            const float tap2 = readInterpolated (
                smearBuffer, smearWrite, d2, ch);

            const float tap3 = readInterpolated (
                smearBuffer, smearWrite, d3, ch);

            const float diffused =
                (tap1 - 0.42f * tap2 + 0.72f * tap3) / 1.72f;

            const float dampAlpha =
                onePoleCoefficient (
                    juce::jmap (smear, 12000.0f, 4200.0f),
                    sr);

            smearDampState[static_cast<size_t> (ch)] +=
                dampAlpha
                * (diffused - smearDampState[static_cast<size_t> (ch)]);

            const float feedback =
                0.12f + smear * 0.56f + chaos * 0.04f * chaosWave;

            smearBuffer[static_cast<size_t> (smearWrite)]
                       [static_cast<size_t> (ch)] =
                x + smearDampState[static_cast<size_t> (ch)]
                    * juce::jlimit (0.0f, 0.72f, feedback);

            const float blurred =
                0.55f * diffused
                + 0.45f * smearDampState[static_cast<size_t> (ch)];

            stage[static_cast<size_t> (ch)] +=
                (blurred - stage[static_cast<size_t> (ch)])
                * (smear * 0.78f);
        }

        matrixSourceTaps[4] = monoTap (stage);

        const float ghostDelayMs =
            juce::jmap (ghost, 150.0f, 610.0f);

        const float ghostDelaySamples =
            ghostDelayMs * static_cast<float> (sr) / 1000.0f;

        const float ghostFeedback =
            juce::jlimit (
                0.0f,
                0.76f,
                0.16f + ghost * 0.50f
                    + chaos * 0.07f * chaosWave);

        for (int ch = 0; ch < 2; ++ch)
        {
            const float x = stage[static_cast<size_t> (ch)];

            const float memory = readInterpolated (
                ghostDelay,
                ghostWrite,
                ghostDelaySamples,
                ch);

            ghostDelay[static_cast<size_t> (ghostWrite)]
                      [static_cast<size_t> (ch)] =
                x + memory * ghostFeedback;

            stage[static_cast<size_t> (ch)] +=
                memory * ghost * 0.52f;
        }

        matrixSourceTaps[3] = monoTap (stage);

        const float airAlpha =
            onePoleCoefficient (4300.0f, sr);

        for (int ch = 0; ch < 2; ++ch)
        {
            const float x = stage[static_cast<size_t> (ch)];

            airLowpassState[static_cast<size_t> (ch)] +=
                airAlpha
                * (x - airLowpassState[static_cast<size_t> (ch)]);

            const float high =
                x - airLowpassState[static_cast<size_t> (ch)];

            stage[static_cast<size_t> (ch)] =
                x + high * air * 0.46f;
        }

        const float mid = 0.5f * (stage[0] + stage[1]);
        const float side = 0.5f * (stage[0] - stage[1])
                         * (1.0f + width * 1.35f);

        const float widthComp = 1.0f / (1.0f + width * 0.22f);

        const float wetL =
            std::tanh ((mid + side) * widthComp);

        const float wetR =
            std::tanh ((mid - side) * widthComp);

        left[i] = dryL + (wetL - dryL) * mix;
        right[i] = dryR + (wetR - dryR) * mix;

        meltWrite =
            (meltWrite + 1) % static_cast<int> (meltDelay.size());

        ghostWrite =
            (ghostWrite + 1) % static_cast<int> (ghostDelay.size());

        grainWrite =
            (grainWrite + 1) % static_cast<int> (grainBuffer.size());

        smearWrite =
            (smearWrite + 1) % static_cast<int> (smearBuffer.size());
    }
}

void IngeniumAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void IngeniumAudioProcessor::setStateInformation (
    const void* data,
    int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
    }
}

juce::AudioProcessorEditor* IngeniumAudioProcessor::createEditor()
{
    return new IngeniumAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new IngeniumAudioProcessor();
}
