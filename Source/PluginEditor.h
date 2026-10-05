#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "AnalogLookAndFeel.h"
#include <array>
#include <memory>

class IngeniumAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit IngeniumAudioProcessorEditor (IngeniumAudioProcessor&);
    ~IngeniumAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment =
        juce::AudioProcessorValueTreeState::SliderAttachment;

    using ComboAttachment =
        juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    void showMainPage();
    void showModPage();
    void updatePageVisibility();
    void styleKnob (juce::Slider& slider);
    void styleLabel (juce::Label& label,
                     const juce::String& text,
                     float size = 12.0f);

    IngeniumAudioProcessor& audioProcessor;
    IngeniumLookAndFeel ingeniumLook;

    bool modPageVisible = false;

    juce::TextButton mainButton { "MAIN" };
    juce::TextButton modButton { "MOD" };

    std::array<juce::Slider, 9> mainKnobs;
    std::array<juce::Label, 9> mainLabels;
    std::array<std::unique_ptr<SliderAttachment>, 9> mainAttachments;

    const std::array<juce::String, 9> mainParameterIDs {
        "age", "melt", "grain",
        "ghost", "smear", "chaos",
        "air", "width", "mix"
    };

    const std::array<juce::String, 9> mainParameterNames {
        "AGE", "MELT", "GRAIN",
        "GHOST", "SMEAR", "CHAOS",
        "AIR", "WIDTH", "MIX"
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

    std::array<juce::Label, 4> matrixRouteLabels;
    std::array<juce::Label, 4> matrixSourceLabels;
    std::array<juce::Label, 4> matrixTargetLabels;
    std::array<juce::Label, 4> matrixModeLabels;
    std::array<juce::Label, 4> matrixAmountLabels;
    std::array<juce::ComboBox, 4> matrixSourceBoxes;
    std::array<juce::ComboBox, 4> matrixTargetBoxes;
    std::array<juce::ComboBox, 4> matrixModeBoxes;
    std::array<juce::Slider, 4> matrixAmountKnobs;

    std::array<std::unique_ptr<ComboAttachment>, 4> matrixSourceAttachments;
    std::array<std::unique_ptr<ComboAttachment>, 4> matrixTargetAttachments;
    std::array<std::unique_ptr<ComboAttachment>, 4> matrixModeAttachments;
    std::array<std::unique_ptr<SliderAttachment>, 4> matrixAmountAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IngeniumAudioProcessorEditor)
};
