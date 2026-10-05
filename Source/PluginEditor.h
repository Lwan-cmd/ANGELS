#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "AnalogLookAndFeel.h"
#include <array>
#include <memory>

class AngelEngineAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit AngelEngineAudioProcessorEditor (AngelEngineAudioProcessor&);
    ~AngelEngineAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    void showMainPage();
    void showModPage();
    void updatePageVisibility();
    void styleKnob (juce::Slider& slider);
    void styleLabel (juce::Label& label, const juce::String& text, float size = 12.0f);

    AngelEngineAudioProcessor& audioProcessor;
    AngelAnalogLookAndFeel analogLook;

    bool modPageVisible = false;

    juce::TextButton mainButton { "MAIN" };
    juce::TextButton modButton { "MOD" };

    std::array<juce::Slider, 7> mainKnobs;
    std::array<juce::Label, 7> mainLabels;
    std::array<std::unique_ptr<SliderAttachment>, 7> mainAttachments;

    const std::array<juce::String, 7> mainParameterIDs {
        "age", "air", "ghost", "width", "melt", "chaos", "mix"
    };

    const std::array<juce::String, 7> mainParameterNames {
        "AGE", "AIR", "GHOST", "WIDTH", "MELT", "CHAOS", "MIX"
    };

    std::array<juce::Slider, 3> rateKnobs;
    std::array<juce::Slider, 3> depthKnobs;
    std::array<juce::Label, 3> lfoLabels;
    std::array<juce::Label, 3> rateLabels;
    std::array<juce::Label, 3> depthLabels;
    std::array<juce::Label, 3> shapeLabels;
    std::array<juce::Label, 3> targetLabels;
    std::array<juce::ComboBox, 3> shapeBoxes;
    std::array<juce::ComboBox, 3> targetBoxes;

    std::array<std::unique_ptr<SliderAttachment>, 3> rateAttachments;
    std::array<std::unique_ptr<SliderAttachment>, 3> depthAttachments;
    std::array<std::unique_ptr<ComboAttachment>, 3> shapeAttachments;
    std::array<std::unique_ptr<ComboAttachment>, 3> targetAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AngelEngineAudioProcessorEditor)
};
