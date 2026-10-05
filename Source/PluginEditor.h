#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "AnalogLookAndFeel.h"
#include <array>
#include <memory>

class IngeniumAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                           private juce::Timer
{
public:
    explicit IngeniumAudioProcessorEditor (IngeniumAudioProcessor&);
    ~IngeniumAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    static constexpr float twoPi = juce::MathConstants<float>::twoPi;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    void timerCallback() override;
    void showMainPage();
    void showModPage();
    void updatePageVisibility();
    void styleKnob (juce::Slider& slider, bool small = false);
    void styleLabel (juce::Label& label, const juce::String& text, float size = 10.0f);
    void styleCombo (juce::ComboBox& box);
    void layoutVerticalModule (juce::Rectangle<int> bounds,
                               int macroIndex,
                               std::initializer_list<int> subIndices,
                               juce::ComboBox* optionalBox = nullptr,
                               juce::Label* optionalLabel = nullptr);

    IngeniumAudioProcessor& audioProcessor;
    IngeniumLookAndFeel ingeniumLook;
    bool modPageVisible = false;

    juce::TextButton mainButton { "MAIN" };
    juce::TextButton modButton  { "MOD" };
    juce::TextButton bypassButton { "BYPASS" };
    std::unique_ptr<ButtonAttachment> bypassAttachment;

    std::array<juce::Slider, 9> mainKnobs;
    std::array<juce::Label, 9> mainLabels;
    std::array<std::unique_ptr<SliderAttachment>, 9> mainAttachments;

    const std::array<juce::String, 9> mainParameterIDs {
        "age", "melt", "grain", "ghost", "smear", "chaos", "air", "width", "mix"
    };
    const std::array<juce::String, 9> mainParameterNames {
        "AGE", "MELT", "GRAIN", "GHOST", "SMEAR", "CHAOS", "AIR", "WIDTH", "MIX"
    };

    std::array<juce::Slider, 24> subKnobs;
    std::array<juce::Label, 24> subLabels;
    std::array<std::unique_ptr<SliderAttachment>, 24> subAttachments;

    const std::array<juce::String, 24> subParameterIDs {
        "ageDrive", "ageWear", "ageTone",
        "meltDepth", "meltRate", "meltVoices",
        "grainSize", "grainDensity", "grainAttack", "grainDelay", "grainSpray", "grainPitch",
        "ghostFeedback", "ghostTone", "ghostDiffusion",
        "smearSize", "smearFeedback", "smearTone", "smearTexture",
        "chaosJitter", "chaosColor",
        "airMid", "airHigh", "widthSpace"
    };

    const std::array<juce::String, 24> subParameterNames {
        "DRIVE", "WEAR", "TONE",
        "DEPTH", "RATE", "VOICES",
        "SIZE", "DENSITY", "ATTACK", "DELAY", "SPRAY", "PITCH",
        "FEEDBACK", "TONE", "DIFFUSION",
        "SIZE", "FEEDBACK", "TONE", "TEXTURE",
        "JITTER", "COLOR",
        "MID", "HIGH", "SPACE"
    };

    juce::ComboBox grainCaptureBox;
    juce::Label grainCaptureLabel;
    std::unique_ptr<ComboAttachment> grainCaptureAttachment;

    juce::ComboBox ghostDivisionBox;
    juce::Label ghostDivisionLabel;
    std::unique_ptr<ComboAttachment> ghostDivisionAttachment;

    std::array<juce::Slider, 3> rateKnobs;
    std::array<juce::Slider, 3> depthKnobs;
    std::array<juce::Label, 3> lfoLabels;
    std::array<juce::Label, 3> rateLabels;
    std::array<juce::Label, 3> depthLabels;
    std::array<juce::Label, 3> shapeLabels;
    std::array<juce::Label, 3> targetLabels;
    std::array<juce::Label, 3> divisionLabels;
    std::array<juce::ComboBox, 3> shapeBoxes;
    std::array<juce::ComboBox, 3> targetBoxes;
    std::array<juce::ComboBox, 3> divisionBoxes;
    std::array<juce::TextButton, 3> syncButtons;

    std::array<std::unique_ptr<SliderAttachment>, 3> rateAttachments;
    std::array<std::unique_ptr<SliderAttachment>, 3> depthAttachments;
    std::array<std::unique_ptr<ComboAttachment>, 3> shapeAttachments;
    std::array<std::unique_ptr<ComboAttachment>, 3> targetAttachments;
    std::array<std::unique_ptr<ComboAttachment>, 3> divisionAttachments;
    std::array<std::unique_ptr<ButtonAttachment>, 3> syncAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IngeniumAudioProcessorEditor)
};