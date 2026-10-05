#include "PluginProcessor.h"
#include "PluginEditor.h"
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
    using Param = juce::AudioParameterFloat;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto addMacro = [&layout] (const char* id, const char* name, float defaultValue)
    {
        layout.add (std::make_unique<Param> (
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

    return layout;
}

void AngelEngineAudioProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;

    const auto meltSize = static_cast<int> (std::ceil (sr * 0.060)) + 4;
    const auto ghostSize = static_cast<int> (std::ceil (sr * 2.0)) + 4;

    meltDelay.assign (static_cast<size_t> (meltSize), { 0.0f, 0.0f });
    ghostDelay.assign (static_cast<size_t> (ghostSize), { 0.0f, 0.0f });

    meltWrite = 0;
    ghostWrite = 0;
    heldSample = { 0.0f, 0.0f };
    holdCounter = { 0, 0 };
    ageLowpassState = { 0.0f, 0.0f };
    airLowpassState = { 0.0f, 0.0f };
    meltPhase = 0.0;
    chaosPhase = 0.0;
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

void AngelEngineAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    if (buffer.getNumChannels() < 2 || meltDelay.empty() || ghostDelay.empty())
        return;

    const float age   = apvts.getRawParameterValue ("age")->load();
    const float air   = apvts.getRawParameterValue ("air")->load();
    const float ghost = apvts.getRawParameterValue ("ghost")->load();
    const float width = apvts.getRawParameterValue ("width")->load();
    const float melt  = apvts.getRawParameterValue ("melt")->load();
    const float chaos = apvts.getRawParameterValue ("chaos")->load();

    auto* left = buffer.getWritePointer (0);
    auto* right = buffer.getWritePointer (1);

    const int numSamples = buffer.getNumSamples();

    const int holdPeriod = 1 + static_cast<int> (std::round (age * age * 12.0f));
    const float ageMix = age * 0.70f;

    const float ageCutoff = juce::jmap (age, 18000.0f, 3500.0f);
    const float ageLPAlpha = onePoleCoefficient (ageCutoff, sr);

    const float airLPAlpha = onePoleCoefficient (4200.0f, sr);
    const float airAmount = air * 0.45f;

    const float ghostDelayMs = juce::jmap (ghost, 180.0f, 520.0f);
    const float ghostDelaySamples = ghostDelayMs * static_cast<float> (sr) / 1000.0f;
    const float ghostFeedbackBase = juce::jmap (ghost, 0.18f, 0.58f);
    const float ghostWet = ghost * 0.50f;

    const float sideGain = 1.0f + width * 1.25f;
    const float widthCompensation = 1.0f / (1.0f + width * 0.22f);

    for (int i = 0; i < numSamples; ++i)
    {
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

        std::array<float, 2> processed {};

        const std::array<float, 2> input { left[i], right[i] };

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

            x = juce::jmap (age * 0.65f,
                            x,
                            ageLowpassState[static_cast<size_t> (ch)]);

            meltDelay[static_cast<size_t> (meltWrite)][static_cast<size_t> (ch)] = x;

            const float meltDelaySamples =
                modulatedDelayMs * static_cast<float> (sr) / 1000.0f;

            const float melted = readInterpolated (
                meltDelay, meltWrite, meltDelaySamples, ch);

            const float meltWet = melt * 0.68f;
            x = juce::jmap (meltWet, x, melted);

            const float drive = 1.0f + melt * 2.8f + chaos * 0.45f;
            x = std::tanh (x * drive) / std::tanh (drive);

            airLowpassState[static_cast<size_t> (ch)] +=
                airLPAlpha * (x - airLowpassState[static_cast<size_t> (ch)]);

            const float high = x - airLowpassState[static_cast<size_t> (ch)];
            x += high * airAmount;

            const float ghostRead = readInterpolated (
                ghostDelay, ghostWrite, ghostDelaySamples, ch);

            const float feedback = juce::jlimit (
                0.0f, 0.74f,
                ghostFeedbackBase + chaos * 0.08f * chaosWave);

            ghostDelay[static_cast<size_t> (ghostWrite)][static_cast<size_t> (ch)] =
                x + ghostRead * feedback;

            x += ghostRead * ghostWet;

            processed[static_cast<size_t> (ch)] = x;
        }

        float mid = 0.5f * (processed[0] + processed[1]);
        float side = 0.5f * (processed[0] - processed[1]) * sideGain;

        left[i]  = std::tanh ((mid + side) * widthCompensation);
        right[i] = std::tanh ((mid - side) * widthCompensation);

        meltWrite = (meltWrite + 1) % static_cast<int> (meltDelay.size());
        ghostWrite = (ghostWrite + 1) % static_cast<int> (ghostDelay.size());
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
