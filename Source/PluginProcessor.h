#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>

class AngelEngineAudioProcessor final : public juce::AudioProcessor
{
public:
    AngelEngineAudioProcessor();
    ~AngelEngineAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    double sr = 44100.0;

    std::vector<std::array<float, 2>> meltDelay;
    std::vector<std::array<float, 2>> ghostDelay;
    int meltWrite = 0;
    int ghostWrite = 0;

    std::array<float, 2> heldSample { 0.0f, 0.0f };
    std::array<int, 2> holdCounter { 0, 0 };
    std::array<float, 2> ageLowpassState { 0.0f, 0.0f };
    std::array<float, 2> airLowpassState { 0.0f, 0.0f };

    double meltPhase = 0.0;
    double chaosPhase = 0.0;

    float readInterpolated (const std::vector<std::array<float, 2>>& buffer,
                            int writePosition,
                            float delaySamples,
                            int channel) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AngelEngineAudioProcessor)
};
