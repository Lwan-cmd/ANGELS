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

    float softSaturate (float x, float drive)
    {
        drive = juce::jmax (1.0f, drive);
        const float normaliser = std::tanh (drive);
        return normaliser > 0.001f ? std::tanh (x * drive) / normaliser : x;
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

    auto add01 = [&layout] (const juce::String& id, const juce::String& name, float defaultValue)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name,
            juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, defaultValue));
    };

    add01 ("age",   "AGE",   0.12f);
    add01 ("melt",  "MELT",  0.12f);
    add01 ("grain", "GRAIN", 0.00f);
    add01 ("ghost", "GHOST", 0.00f);
    add01 ("smear", "SMEAR", 0.00f);
    add01 ("chaos", "CHAOS", 0.00f);
    add01 ("air",   "AIR",   0.10f);
    add01 ("width", "WIDTH", 0.10f);
    add01 ("mix",   "MIX",   1.00f);

    add01 ("ageDrive", "AGE DRIVE", 0.35f);
    add01 ("ageWear",  "AGE WEAR",  0.15f);
    add01 ("ageTone",  "AGE TONE",  0.58f);

    add01 ("meltDepth",  "MELT DEPTH",  0.40f);
    add01 ("meltRate",   "MELT RATE",   0.22f);
    add01 ("meltVoices", "MELT VOICES", 0.55f);

    add01 ("grainSize",    "GRAIN SIZE",    0.58f);
    add01 ("grainDensity", "GRAIN DENSITY", 0.62f);
    add01 ("grainAttack",  "GRAIN ATTACK",  0.28f);
    add01 ("grainDelay",   "GRAIN DELAY",   0.30f);
    add01 ("grainSpray",   "GRAIN SPRAY",   0.18f);
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "grainPitch", 1 }, "GRAIN PITCH",
        juce::NormalisableRange<float> { -12.0f, 12.0f, 0.01f }, 0.0f));
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "grainCapture", 1 }, "GRAIN CAPTURE",
        juce::StringArray { "FLOW", "TRANSIENT", "GRID" }, 0));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "ghostDivision", 1 }, "GHOST DIVISION",
        juce::StringArray { "1/4", "1/8", "1/8D", "1/8T", "1/16" }, 1));
    add01 ("ghostFeedback",  "GHOST FEEDBACK",  0.36f);
    add01 ("ghostTone",      "GHOST TONE",      0.56f);
    add01 ("ghostDiffusion", "GHOST DIFFUSION", 0.34f);

    add01 ("smearSize",     "SMEAR SIZE",     0.46f);
    add01 ("smearFeedback", "SMEAR FEEDBACK", 0.34f);
    add01 ("smearTone",     "SMEAR TONE",     0.55f);
    add01 ("smearTexture",  "SMEAR TEXTURE",  0.38f);

    add01 ("chaosJitter", "CHAOS JITTER", 0.52f);
    add01 ("chaosColor",  "CHAOS COLOR",  0.48f);

    add01 ("airMid",  "AIR MID",  0.55f);
    add01 ("airHigh", "AIR HIGH", 0.68f);
    add01 ("widthSpace", "WIDTH SPACE", 0.42f);

    const juce::StringArray shapes { "SINE", "TRIANGLE", "SAW", "SQUARE", "RANDOM" };
    const juce::StringArray destinations { "NONE", "AGE", "MELT", "GRAIN", "GHOST", "SMEAR", "CHAOS" };
    const juce::StringArray divisions { "1/1", "1/2", "1/4", "1/8", "1/8D", "1/8T", "1/16", "1/32" };

    for (int i = 1; i <= 3; ++i)
    {
        const auto n = juce::String (i);
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "lfo" + n + "Rate", 1 }, "LFO " + n + " RATE",
            juce::NormalisableRange<float> { 0.03f, 12.0f, 0.001f, 0.35f },
            0.20f * static_cast<float> (i)));
        add01 ("lfo" + n + "Depth", "LFO " + n + " DEPTH", 0.0f);
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "lfo" + n + "Shape", 1 }, "LFO " + n + " SHAPE", shapes, i == 3 ? 4 : 0));
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "lfo" + n + "Dest", 1 }, "LFO " + n + " DESTINATION", destinations, 0));
        layout.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { "lfo" + n + "Sync", 1 }, "LFO " + n + " SYNC", false));
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "lfo" + n + "Division", 1 }, "LFO " + n + " DIVISION", divisions, 3));
    }

    return layout;
}

void IngeniumAudioProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    meltDelay.assign   (static_cast<size_t> (std::ceil (sr * 0.090)) + 8, { 0.0f, 0.0f });
    ghostDelay.assign  (static_cast<size_t> (std::ceil (sr * 5.0)) + 8,   { 0.0f, 0.0f });
    grainBuffer.assign (static_cast<size_t> (std::ceil (sr * 4.0)) + 8,   { 0.0f, 0.0f });
    smearBuffer.assign (static_cast<size_t> (std::ceil (sr * 0.260)) + 8, { 0.0f, 0.0f });
    chaosDelay.assign  (static_cast<size_t> (std::ceil (sr * 0.080)) + 8, { 0.0f, 0.0f });

    meltWrite = ghostWrite = grainWrite = smearWrite = chaosWrite = 0;
    ageLossState = { 0.0f, 0.0f };
    ageBassState = { 0.0f, 0.0f };
    ghostToneState = { 0.0f, 0.0f };
    smearDampState = { 0.0f, 0.0f };
    chaosToneState = { 0.0f, 0.0f };
    airMidLowState = { 0.0f, 0.0f };
    airHighLowState = { 0.0f, 0.0f };
    airEnvelope = { 0.0f, 0.0f };
    widthSideLowState = 0.0f;

    meltPhaseA = 0.0;
    meltPhaseB = 1.7;
    ghostPhase = 0.0;
    lfoPhase = { 0.0, 0.0, 0.0 };
    grains = {};
    samplesUntilNextGrain = 0;
    grainTransientEnvelope = 0.0f;
    lastTransientWrite = 0;
    samplesSinceTransient = 0;
    ageWearCounter = 0;
    ageWearTarget = ageWearSmooth = 0.0f;
    chaosRandomCounter = 0;
    chaosTarget = chaosSmooth = 0.0f;
}

bool IngeniumAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

float IngeniumAudioProcessor::readInterpolated (const std::vector<std::array<float, 2>>& buffer,
                                                int writePosition,
                                                float delaySamples,
                                                int channel) const
{
    if (buffer.empty()) return 0.0f;
    const int size = static_cast<int> (buffer.size());
    float readPosition = static_cast<float> (writePosition) - delaySamples;
    while (readPosition < 0.0f) readPosition += static_cast<float> (size);
    while (readPosition >= static_cast<float> (size)) readPosition -= static_cast<float> (size);
    const int i0 = static_cast<int> (std::floor (readPosition)) % size;
    const int i1 = (i0 + 1) % size;
    const float frac = readPosition - std::floor (readPosition);
    const float a = buffer[static_cast<size_t> (i0)][static_cast<size_t> (channel)];
    const float b = buffer[static_cast<size_t> (i1)][static_cast<size_t> (channel)];
    return a + (b - a) * frac;
}

float IngeniumAudioProcessor::readGrainAt (double position, int channel) const
{
    if (grainBuffer.empty()) return 0.0f;
    const int size = static_cast<int> (grainBuffer.size());
    while (position < 0.0) position += static_cast<double> (size);
    while (position >= static_cast<double> (size)) position -= static_cast<double> (size);
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
    return static_cast<float> (grainRandomState & 0x00ffffffu) / static_cast<float> (0x00ffffffu);
}

float IngeniumAudioProcessor::nextTextureRandom()
{
    textureRandomState = textureRandomState * 1664525u + 1013904223u;
    return static_cast<float> (textureRandomState & 0x00ffffffu) / static_cast<float> (0x00ffffffu);
}

void IngeniumAudioProcessor::spawnGrain (float amount, float size, float attack, float delay,
                                         float spray, float pitchSemitones, int captureMode, float chaos)
{
    if (grainBuffer.empty() || amount <= 0.001f) return;

    GrainVoice* voice = nullptr;
    for (auto& candidate : grains)
        if (! candidate.active) { voice = &candidate; break; }

    if (voice == nullptr)
        voice = &*std::max_element (grains.begin(), grains.end(),
            [] (const GrainVoice& a, const GrainVoice& b) { return a.ageSamples < b.ageSamples; });

    const float r1 = nextGrainRandom();
    const float r2 = nextGrainRandom();
    const float r3 = nextGrainRandom();
    const float r4 = nextGrainRandom();
    const float lengthMs = juce::jmap (size, 35.0f, 520.0f) * (0.82f + 0.36f * r1);
    const float baseDelayMs = juce::jmap (delay, 18.0f, 1200.0f);
    const float sprayMs = spray * spray * 850.0f * (r2 * 2.0f - 1.0f);

    double startPosition = static_cast<double> (grainWrite)
                         - static_cast<double> ((baseDelayMs + sprayMs) * static_cast<float> (sr) / 1000.0f);

    if (captureMode == 1)
        startPosition = static_cast<double> (lastTransientWrite)
                      - static_cast<double> (sprayMs * 0.35f * static_cast<float> (sr) / 1000.0f);
    else if (captureMode == 2)
    {
        const double sixteenth = sr * 60.0 / juce::jmax (40.0, currentBpm) / 4.0;
        const double lookback = static_cast<double> (grainWrite) - startPosition;
        startPosition = static_cast<double> (grainWrite)
                      - std::round (lookback / juce::jmax (1.0, sixteenth)) * sixteenth;
    }

    float semitones = pitchSemitones + (r3 * 2.0f - 1.0f) * spray * 4.5f
                    + chaos * (r4 * 2.0f - 1.0f) * 1.5f;
    semitones = juce::jlimit (-24.0f, 24.0f, semitones);
    const double ratio = std::pow (2.0, static_cast<double> (semitones) / 12.0);
    const bool reverse = r4 < (0.015f + spray * 0.13f + chaos * 0.04f);

    voice->active = true;
    voice->readPosition = startPosition;
    voice->increment = reverse ? -ratio : ratio;
    voice->ageSamples = 0;
    voice->lengthSamples = juce::jmax (32, static_cast<int> (lengthMs * static_cast<float> (sr) / 1000.0f));
    voice->attackFraction = juce::jmap (attack, 0.035f, 0.46f);
    voice->pan = (nextGrainRandom() * 2.0f - 1.0f) * (0.12f + spray * 0.68f);
    voice->gain = 0.72f + 0.18f * (1.0f - spray);
}

float IngeniumAudioProcessor::getLfoValue (int lfoIndex, int shape) const
{
    const float norm = static_cast<float> (lfoPhase[static_cast<size_t> (lfoIndex)]
                                            / juce::MathConstants<double>::twoPi);
    switch (shape)
    {
        case 1:
            if (norm < 0.25f) return norm * 4.0f;
            if (norm < 0.75f) return 2.0f - norm * 4.0f;
            return norm * 4.0f - 4.0f;
        case 2: return norm * 2.0f - 1.0f;
        case 3: return norm < 0.5f ? 1.0f : -1.0f;
        case 4: return randomHeld[static_cast<size_t> (lfoIndex)];
        default: return std::sin (static_cast<float> (lfoPhase[static_cast<size_t> (lfoIndex)]));
    }
}

void IngeniumAudioProcessor::advanceLfo (int lfoIndex, float rateHz)
{
    auto& phase = lfoPhase[static_cast<size_t> (lfoIndex)];
    phase += juce::MathConstants<double>::twoPi * static_cast<double> (rateHz) / sr;
    if (phase >= juce::MathConstants<double>::twoPi)
    {
        phase -= juce::MathConstants<double>::twoPi;
        auto& state = randomState[static_cast<size_t> (lfoIndex)];
        state = state * 1664525u + 1013904223u;
        const float unit = static_cast<float> (state & 0x00ffffffu) / static_cast<float> (0x00ffffffu);
        randomHeld[static_cast<size_t> (lfoIndex)] = unit * 2.0f - 1.0f;
    }
}

float IngeniumAudioProcessor::getSyncedLfoRate (int divisionIndex) const
{
    const double beatsPerCycle[] { 4.0, 2.0, 1.0, 0.5, 0.75, 1.0 / 3.0, 0.25, 0.125 };
    const int index = juce::jlimit (0, 7, divisionIndex);
    return static_cast<float> ((currentBpm / 60.0) / beatsPerCycle[index]);
}

float IngeniumAudioProcessor::getGhostBeatFactor (int divisionIndex) const
{
    const float factors[] { 1.0f, 0.5f, 0.75f, 1.0f / 3.0f, 0.25f };
    return factors[juce::jlimit (0, 4, divisionIndex)];
}

void IngeniumAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    if (buffer.getNumChannels() < 2 || meltDelay.empty() || ghostDelay.empty() || grainBuffer.empty()
        || smearBuffer.empty() || chaosDelay.empty()) return;

    if (auto* playHead = getPlayHead())
        if (auto position = playHead->getPosition())
            if (auto bpm = position->getBpm())
                currentBpm = juce::jlimit (40.0, 300.0, *bpm);

    const std::array<float, 6> base {
        apvts.getRawParameterValue ("age")->load(), apvts.getRawParameterValue ("melt")->load(),
        apvts.getRawParameterValue ("grain")->load(), apvts.getRawParameterValue ("ghost")->load(),
        apvts.getRawParameterValue ("smear")->load(), apvts.getRawParameterValue ("chaos")->load()
    };

    const float air = apvts.getRawParameterValue ("air")->load();
    const float width = apvts.getRawParameterValue ("width")->load();
    const float mix = apvts.getRawParameterValue ("mix")->load();
    const float ageDrive = apvts.getRawParameterValue ("ageDrive")->load();
    const float ageWear = apvts.getRawParameterValue ("ageWear")->load();
    const float ageTone = apvts.getRawParameterValue ("ageTone")->load();
    const float meltDepth = apvts.getRawParameterValue ("meltDepth")->load();
    const float meltRate = apvts.getRawParameterValue ("meltRate")->load();
    const float meltVoices = apvts.getRawParameterValue ("meltVoices")->load();
    const float grainSize = apvts.getRawParameterValue ("grainSize")->load();
    const float grainDensity = apvts.getRawParameterValue ("grainDensity")->load();
    const float grainAttack = apvts.getRawParameterValue ("grainAttack")->load();
    const float grainDelay = apvts.getRawParameterValue ("grainDelay")->load();
    const float grainSpray = apvts.getRawParameterValue ("grainSpray")->load();
    const float grainPitch = apvts.getRawParameterValue ("grainPitch")->load();
    const int grainCapture = static_cast<int> (std::round (apvts.getRawParameterValue ("grainCapture")->load()));
    const int ghostDivision = static_cast<int> (std::round (apvts.getRawParameterValue ("ghostDivision")->load()));
    const float ghostFeedback = apvts.getRawParameterValue ("ghostFeedback")->load();
    const float ghostTone = apvts.getRawParameterValue ("ghostTone")->load();
    const float ghostDiffusion = apvts.getRawParameterValue ("ghostDiffusion")->load();
    const float smearSize = apvts.getRawParameterValue ("smearSize")->load();
    const float smearFeedback = apvts.getRawParameterValue ("smearFeedback")->load();
    const float smearTone = apvts.getRawParameterValue ("smearTone")->load();
    const float smearTexture = apvts.getRawParameterValue ("smearTexture")->load();
    const float chaosJitter = apvts.getRawParameterValue ("chaosJitter")->load();
    const float chaosColor = apvts.getRawParameterValue ("chaosColor")->load();
    const float airMid = apvts.getRawParameterValue ("airMid")->load();
    const float airHigh = apvts.getRawParameterValue ("airHigh")->load();
    const float widthSpace = apvts.getRawParameterValue ("widthSpace")->load();

    std::array<float, 3> lfoRates {}, lfoDepths {};
    std::array<int, 3> lfoShapes {}, lfoDests {}, lfoDivisions {};
    std::array<bool, 3> lfoSync {};
    for (int lfo = 0; lfo < 3; ++lfo)
    {
        const auto n = juce::String (lfo + 1);
        lfoRates[lfo] = apvts.getRawParameterValue ("lfo" + n + "Rate")->load();
        lfoDepths[lfo] = apvts.getRawParameterValue ("lfo" + n + "Depth")->load();
        lfoShapes[lfo] = static_cast<int> (std::round (apvts.getRawParameterValue ("lfo" + n + "Shape")->load()));
        lfoDests[lfo] = static_cast<int> (std::round (apvts.getRawParameterValue ("lfo" + n + "Dest")->load()));
        lfoSync[lfo] = apvts.getRawParameterValue ("lfo" + n + "Sync")->load() >= 0.5f;
        lfoDivisions[lfo] = static_cast<int> (std::round (apvts.getRawParameterValue ("lfo" + n + "Division")->load()));
    }

    auto* left = buffer.getWritePointer (0);
    auto* right = buffer.getWritePointer (1);
    const int numSamples = buffer.getNumSamples();
    const float wearSmoothCoeff = onePoleCoefficient (6.0f, sr);
    const float chaosSmoothCoeff = onePoleCoefficient (32.0f, sr);
    const float transientRelease = onePoleCoefficient (16.0f, sr);

    for (int i = 0; i < numSamples; ++i)
    {
        std::array<float, 6> modulated = base;
        for (int lfo = 0; lfo < 3; ++lfo)
        {
            const float value = getLfoValue (lfo, lfoShapes[lfo]);
            const int dest = lfoDests[lfo];
            if (dest >= 1 && dest <= 6)
            {
                auto& target = modulated[static_cast<size_t> (dest - 1)];
                target = juce::jlimit (0.0f, 1.0f, target + value * lfoDepths[lfo] * 0.88f);
            }
            const float rate = lfoSync[lfo] ? getSyncedLfoRate (lfoDivisions[lfo]) : lfoRates[lfo];
            advanceLfo (lfo, rate);
        }

        const float age = modulated[0];
        const float melt = modulated[1];
        const float grain = modulated[2];
        const float ghost = modulated[3];
        const float smear = modulated[4];
        const float chaos = modulated[5];
        const float dryL = left[i];
        const float dryR = right[i];
        std::array<float, 2> stage { dryL, dryR };

        if (--ageWearCounter <= 0)
        {
            ageWearTarget = nextTextureRandom() * 2.0f - 1.0f;
            ageWearCounter = juce::jmax (32, static_cast<int> (sr * (0.030 + nextTextureRandom() * 0.120)));
        }
        ageWearSmooth += wearSmoothCoeff * (ageWearTarget - ageWearSmooth);
        const float ageLossCutoff = juce::jmap (ageTone, 6200.0f, 19500.0f) * (1.0f - age * 0.28f);
        const float ageLossAlpha = onePoleCoefficient (ageLossCutoff, sr);
        const float ageBassAlpha = onePoleCoefficient (115.0f, sr);
        const float ageDriveAmount = 1.0f + ageDrive * 4.2f + age * 1.4f;
        const float wearGain = 1.0f - age * ageWear * (0.018f + 0.045f * (ageWearSmooth * 0.5f + 0.5f));
        for (int ch = 0; ch < 2; ++ch)
        {
            const float x = stage[ch];
            ageLossState[ch] += ageLossAlpha * (x - ageLossState[ch]);
            ageBassState[ch] += ageBassAlpha * (x - ageBassState[ch]);
            const float coloured = ageLossState[ch] + ageBassState[ch] * age * (0.08f + 0.08f * (1.0f - ageTone));
            const float bias = (ageTone - 0.5f) * 0.035f;
            const float sat = softSaturate (coloured + bias, ageDriveAmount) - softSaturate (bias, ageDriveAmount);
            const float tape = (0.35f * coloured + 0.65f * sat) * wearGain;
            stage[ch] = x + (tape - x) * age;
        }

        const float meltHz = juce::jmap (meltRate, 0.035f, 1.15f);
        const float depthMs = (0.25f + meltDepth * 8.5f) * (0.55f + 0.75f * melt);
        const float voiceSpread = 0.25f + meltVoices * 0.75f;
        meltPhaseA += twoPi * meltHz / sr;
        meltPhaseB += twoPi * meltHz * (0.61 + 0.28 * meltVoices) / sr;
        if (meltPhaseA >= twoPi) meltPhaseA -= twoPi;
        if (meltPhaseB >= twoPi) meltPhaseB -= twoPi;
        for (int ch = 0; ch < 2; ++ch)
            meltDelay[static_cast<size_t> (meltWrite)][static_cast<size_t> (ch)] = stage[ch];
        for (int ch = 0; ch < 2; ++ch)
        {
            const float stereoPhase = ch == 0 ? 0.0f : 1.5708f;
            const float a = std::sin (static_cast<float> (meltPhaseA) + stereoPhase);
            const float b = std::sin (static_cast<float> (meltPhaseB) - stereoPhase * 0.7f);
            const float d1 = 7.0f + depthMs * (0.52f + 0.48f * a);
            const float d2 = 13.0f + depthMs * voiceSpread * (0.50f + 0.50f * b);
            const float v1 = readInterpolated (meltDelay, meltWrite, d1 * static_cast<float> (sr) / 1000.0f, ch);
            const float v2 = readInterpolated (meltDelay, meltWrite, d2 * static_cast<float> (sr) / 1000.0f, ch);
            const float chorus = v1 * (0.68f - 0.20f * meltVoices) + v2 * (0.32f + 0.20f * meltVoices);
            const float wet = melt * 0.82f;
            stage[ch] = stage[ch] * (1.0f - wet * 0.48f) + chorus * wet * 0.76f;
        }

        const float monoAbs = std::abs (0.5f * (stage[0] + stage[1]));
        grainTransientEnvelope += transientRelease * (monoAbs - grainTransientEnvelope);
        ++samplesSinceTransient;
        if (monoAbs > grainTransientEnvelope * 1.75f + 0.025f && samplesSinceTransient > static_cast<int> (sr * 0.035))
        {
            lastTransientWrite = grainWrite;
            samplesSinceTransient = 0;
        }
        grainBuffer[static_cast<size_t> (grainWrite)] = stage;
        if (grain > 0.001f)
        {
            --samplesUntilNextGrain;
            if (samplesUntilNextGrain <= 0)
            {
                spawnGrain (grain, grainSize, grainAttack, grainDelay, grainSpray, grainPitch, grainCapture, chaos);
                const float densityHz = 5.0f + grainDensity * grainDensity * 82.0f;
                samplesUntilNextGrain = juce::jmax (1, static_cast<int> (sr / densityHz));
            }
        }
        else samplesUntilNextGrain = 0;

        std::array<float, 2> grainSum { 0.0f, 0.0f };
        int activeGrains = 0;
        for (auto& voice : grains)
        {
            if (! voice.active) continue;
            const float pos = static_cast<float> (voice.ageSamples) / static_cast<float> (juce::jmax (1, voice.lengthSamples));
            if (pos >= 1.0f) { voice.active = false; continue; }
            const float attackFrac = juce::jlimit (0.02f, 0.48f, voice.attackFraction);
            float env = 1.0f;
            if (pos < attackFrac)
            {
                const float p = pos / attackFrac;
                env = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::pi * p);
            }
            else
            {
                const float p = (pos - attackFrac) / juce::jmax (0.001f, 1.0f - attackFrac);
                env = 0.5f + 0.5f * std::cos (juce::MathConstants<float>::pi * p);
            }
            const float panL = std::sqrt (0.5f * juce::jlimit (0.0f, 2.0f, 1.0f - voice.pan));
            const float panR = std::sqrt (0.5f * juce::jlimit (0.0f, 2.0f, 1.0f + voice.pan));
            grainSum[0] += readGrainAt (voice.readPosition, 0) * env * voice.gain * panL;
            grainSum[1] += readGrainAt (voice.readPosition, 1) * env * voice.gain * panR;
            voice.readPosition += voice.increment;
            ++voice.ageSamples;
            ++activeGrains;
        }
        if (activeGrains > 0)
        {
            const float norm = 1.0f / std::sqrt (static_cast<float> (activeGrains));
            const float wet = grain * 0.88f;
            stage[0] = stage[0] * (1.0f - wet * 0.62f) + grainSum[0] * norm * wet * 0.92f;
            stage[1] = stage[1] * (1.0f - wet * 0.62f) + grainSum[1] * norm * wet * 0.92f;
        }

        const float smearScale = 0.55f + smearSize * 1.45f;
        const float smearToneAlpha = onePoleCoefficient (juce::jmap (smearTone, 2600.0f, 15800.0f), sr);
        std::array<float, 2> smeared {};
        for (int ch = 0; ch < 2; ++ch)
        {
            const float t1 = readInterpolated (smearBuffer, smearWrite, 4.3f * smearScale * static_cast<float> (sr) / 1000.0f, ch);
            const float t2 = readInterpolated (smearBuffer, smearWrite, 13.7f * smearScale * static_cast<float> (sr) / 1000.0f, ch);
            const float t3 = readInterpolated (smearBuffer, smearWrite, 31.1f * smearScale * static_cast<float> (sr) / 1000.0f, ch);
            const float t4 = readInterpolated (smearBuffer, smearWrite, 61.0f * smearScale * static_cast<float> (sr) / 1000.0f, 1 - ch);
            float diffuse = (t1 - 0.48f * t2 + 0.36f * t3 + t4 * smearTexture * 0.55f) / (1.55f + smearTexture * 0.55f);
            diffuse = softSaturate (diffuse, 1.0f + smearTexture * 2.3f);
            smearDampState[ch] += smearToneAlpha * (diffuse - smearDampState[ch]);
            smeared[ch] = 0.48f * diffuse + 0.52f * smearDampState[ch];
        }
        for (int ch = 0; ch < 2; ++ch)
        {
            const float feedback = juce::jlimit (0.0f, 0.86f, 0.05f + smearFeedback * 0.78f);
            smearBuffer[static_cast<size_t> (smearWrite)][static_cast<size_t> (ch)] = stage[ch] + smeared[ch] * feedback;
            stage[ch] += (smeared[ch] - stage[ch]) * (smear * 0.92f);
        }

        const float quarterMs = static_cast<float> (60000.0 / juce::jmax (40.0, currentBpm));
        const float ghostDrift = 1.0f + std::sin (static_cast<float> (ghostPhase)) * (0.0015f + ghostDiffusion * 0.0045f);
        ghostPhase += twoPi * (0.035 + ghostDiffusion * 0.055) / sr;
        if (ghostPhase >= twoPi) ghostPhase -= twoPi;
        const float baseGhostSamples = juce::jlimit (32.0f, static_cast<float> (ghostDelay.size() - 8),
            quarterMs * getGhostBeatFactor (ghostDivision) * ghostDrift * static_cast<float> (sr) / 1000.0f);
        std::array<float, 2> ghostWetSignal {};
        for (int ch = 0; ch < 2; ++ch)
        {
            const float a = readInterpolated (ghostDelay, ghostWrite, baseGhostSamples, ch);
            const float b = readInterpolated (ghostDelay, ghostWrite, baseGhostSamples * 1.50f, 1 - ch);
            const float c = readInterpolated (ghostDelay, ghostWrite, baseGhostSamples * 0.74f + 17.0f * static_cast<float> (sr) / 1000.0f, ch);
            ghostWetSignal[ch] = a * (0.76f - 0.22f * ghostDiffusion)
                               + b * (0.10f + 0.28f * ghostDiffusion)
                               + c * (0.14f + 0.18f * ghostDiffusion);
        }
        const float ghostToneAlpha = onePoleCoefficient (juce::jmap (ghostTone, 1600.0f, 14500.0f), sr);
        for (int ch = 0; ch < 2; ++ch)
            ghostToneState[ch] += ghostToneAlpha * (ghostWetSignal[ch] - ghostToneState[ch]);
        for (int ch = 0; ch < 2; ++ch)
        {
            const float cross = ghostToneState[1 - ch];
            const float feedbackSignal = ghostToneState[ch] * (1.0f - 0.38f * ghostDiffusion) + cross * (0.38f * ghostDiffusion);
            ghostDelay[static_cast<size_t> (ghostWrite)][static_cast<size_t> (ch)] = stage[ch]
                + feedbackSignal * juce::jlimit (0.0f, 0.88f, ghostFeedback * 0.86f);
            stage[ch] += ghostWetSignal[ch] * ghost * 0.82f;
        }

        if (--chaosRandomCounter <= 0)
        {
            chaosTarget = nextTextureRandom() * 2.0f - 1.0f;
            chaosRandomCounter = juce::jmax (8, static_cast<int> (sr / (5.0f + chaosJitter * 72.0f)));
        }
        chaosSmooth += chaosSmoothCoeff * (chaosTarget - chaosSmooth);
        for (int ch = 0; ch < 2; ++ch)
            chaosDelay[static_cast<size_t> (chaosWrite)][static_cast<size_t> (ch)] = stage[ch];
        const float chaosAlpha = onePoleCoefficient (juce::jmap (chaosColor, 17500.0f, 3900.0f), sr);
        for (int ch = 0; ch < 2; ++ch)
        {
            const float polarity = ch == 0 ? 1.0f : -0.72f;
            const float jitterMs = 0.25f + chaosJitter * chaos * (1.4f + 9.0f * std::abs (chaosSmooth));
            const float delayed = readInterpolated (chaosDelay, chaosWrite,
                jitterMs * static_cast<float> (sr) / 1000.0f, ch);
            chaosToneState[ch] += chaosAlpha * (delayed - chaosToneState[ch]);
            const float coloured = delayed + (delayed - chaosToneState[ch]) * chaosColor * 0.42f * polarity;
            const float unstable = softSaturate (coloured * (1.0f + chaos * chaosColor * 0.65f),
                                                  1.0f + chaos * chaosColor * 1.8f);
            stage[ch] += (unstable - stage[ch]) * chaos * (0.42f + chaosJitter * 0.38f);
        }

        const float midAlpha = onePoleCoefficient (2700.0f, sr);
        const float highAlpha = onePoleCoefficient (8200.0f, sr);
        const float envAlpha = onePoleCoefficient (35.0f, sr);
        for (int ch = 0; ch < 2; ++ch)
        {
            const float x = stage[ch];
            airMidLowState[ch] += midAlpha * (x - airMidLowState[ch]);
            airHighLowState[ch] += highAlpha * (x - airHighLowState[ch]);
            const float midBand = x - airMidLowState[ch];
            const float highBand = x - airHighLowState[ch];
            airEnvelope[ch] += envAlpha * (std::abs (highBand) - airEnvelope[ch]);
            const float dynamic = 1.0f / (1.0f + airEnvelope[ch] * 3.5f);
            const float midLift = softSaturate (midBand, 1.35f) * air * airMid * 0.72f;
            const float highLift = softSaturate (highBand, 1.55f) * air * airHigh * (0.92f + 0.36f * dynamic);
            stage[ch] = x + midLift + highLift;
        }

        const float mid = 0.5f * (stage[0] + stage[1]);
        float side = 0.5f * (stage[0] - stage[1]);
        const float sideAlpha = onePoleCoefficient (2600.0f, sr);
        widthSideLowState += sideAlpha * (side - widthSideLowState);
        const float sideHigh = side - widthSideLowState;
        side *= 1.0f + width * 1.95f;
        side += sideHigh * width * widthSpace * 1.55f;
        const float widthComp = 1.0f / (1.0f + width * 0.20f);
        const float wetL = (mid + side) * widthComp;
        const float wetR = (mid - side) * widthComp;

        left[i] = dryL + (wetL - dryL) * mix;
        right[i] = dryR + (wetR - dryR) * mix;

        meltWrite = (meltWrite + 1) % static_cast<int> (meltDelay.size());
        ghostWrite = (ghostWrite + 1) % static_cast<int> (ghostDelay.size());
        grainWrite = (grainWrite + 1) % static_cast<int> (grainBuffer.size());
        smearWrite = (smearWrite + 1) % static_cast<int> (smearBuffer.size());
        chaosWrite = (chaosWrite + 1) % static_cast<int> (chaosDelay.size());
    }
}

void IngeniumAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, destData);
}

void IngeniumAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType())) apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* IngeniumAudioProcessor::createEditor()
{
    return new IngeniumAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new IngeniumAudioProcessor();
}
