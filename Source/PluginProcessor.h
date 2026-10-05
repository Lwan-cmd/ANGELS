#pragma once

#include <JuceHeader.h>
#include <array>
#include <cstdint>
#include <vector>

class IngeniumAudioProcessor final : public juce::AudioProcessor
{
public:
    IngeniumAudioProcessor();
    ~IngeniumAudioProcessor() override = default;

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
    double getTailLengthSeconds() const override { return 5.0; }

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
    struct GrainVoice
    {
        bool active = false;
        double readPosition = 0.0;
        double increment = 1.0;
        int ageSamples = 0;
        int lengthSamples = 1;
        float attackFraction = 0.15f;
        float pan = 0.0f;
        float gain = 1.0f;
    };

    double sr = 44100.0;
    double currentBpm = 120.0;

    std::vector<std::array<float, 2>> meltDelay;
    std::vector<std::array<float, 2>> ghostDelay;
    std::vector<std::array<float, 2>> grainBuffer;
    std::vector<std::array<float, 2>> smearBuffer;
    std::vector<std::array<float, 2>> chaosDelay;

    int meltWrite = 0;
    int ghostWrite = 0;
    int grainWrite = 0;
    int smearWrite = 0;
    int chaosWrite = 0;

    std::array<float, 2> ageLossState { 0.0f, 0.0f };
    std::array<float, 2> ageBassState { 0.0f, 0.0f };
    std::array<float, 2> ghostToneState { 0.0f, 0.0f };
    std::array<float, 2> smearDampState { 0.0f, 0.0f };
    std::array<float, 2> chaosToneState { 0.0f, 0.0f };
    std::array<float, 2> airMidLowState { 0.0f, 0.0f };
    std::array<float, 2> airHighLowState { 0.0f, 0.0f };
    std::array<float, 2> airEnvelope { 0.0f, 0.0f };
    float widthSideLowState = 0.0f;

    double meltPhaseA = 0.0;
    double meltPhaseB = 1.7;
    double ghostPhase = 0.0;

    std::array<double, 3> lfoPhase { 0.0, 0.0, 0.0 };
    std::array<float, 3> randomHeld { 0.15f, -0.37f, 0.62f };
    std::array<std::uint32_t, 3> randomState { 0x12345678u, 0x87654321u, 0x31415926u };

    std::array<GrainVoice, 20> grains;
    int samplesUntilNextGrain = 0;
    std::uint32_t grainRandomState = 0x9e3779b9u;
    float grainTransientEnvelope = 0.0f;
    int lastTransientWrite = 0;
    int samplesSinceTransient = 0;

    std::uint32_t textureRandomState = 0x6d2b79f5u;
    int ageWearCounter = 0;
    float ageWearTarget = 0.0f;
    float ageWearSmooth = 0.0f;
    int chaosRandomCounter = 0;
    float chaosTarget = 0.0f;
    float chaosSmooth = 0.0f;

    float readInterpolated (const std::vector<std::array<float, 2>>& buffer,
                            int writePosition,
                            float delaySamples,
                            int channel) const;
    float readGrainAt (double position, int channel) const;
    float nextGrainRandom();
    float nextTextureRandom();

    void spawnGrain (float amount,
                     float size,
                     float attack,
                     float delay,
                     float spray,
                     float pitchSemitones,
                     int captureMode,
                     float chaos);

    float getLfoValue (int lfoIndex, int shape) const;
    void advanceLfo (int lfoIndex, float rateHz);
    float getSyncedLfoRate (int divisionIndex) const;
    float getGhostBeatFactor (int divisionIndex) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IngeniumAudioProcessor)
};
