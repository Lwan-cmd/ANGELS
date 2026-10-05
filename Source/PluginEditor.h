#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>
#include <memory>

class AngelEngineAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit AngelEngineAudioProcessorEditor (AngelEngineAudioProcessor&);
    ~AngelEngineAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    AngelEngineAudioProcessor& processor;

    std::array<juce::Slider, 6> knobs;
    std::array<juce::Label, 6> labels;
    std::array<std::unique_ptr<Attachment>, 6> attachments;

    const std::array<juce::String, 6> parameterIDs {
        "age", "air", "ghost", "width", "melt", "chaos"
    };

    const std::array<juce::String, 6> parameterNames {
        "AGE", "AIR", "GHOST", "WIDTH", "MELT", "CHAOS"
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AngelEngineAudioProcessorEditor)
};
