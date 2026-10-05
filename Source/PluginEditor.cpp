#include "PluginEditor.h"

AngelEngineAudioProcessorEditor::AngelEngineAudioProcessorEditor (
    AngelEngineAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p)
{
    setSize (760, 360);
    setResizable (true, true);
    setResizeLimits (620, 300, 1100, 520);

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto& knob = knobs[i];
        knob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        knob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 62, 20);
        knob.setRange (0.0, 1.0, 0.001);
        knob.setDoubleClickReturnValue (true, 0.0);
        knob.setColour (juce::Slider::rotarySliderFillColourId,
                        juce::Colour::fromRGB (112, 76, 255));
        knob.setColour (juce::Slider::rotarySliderOutlineColourId,
                        juce::Colour::fromRGB (225, 225, 230));
        knob.setColour (juce::Slider::thumbColourId,
                        juce::Colour::fromRGB (30, 30, 34));
        knob.setColour (juce::Slider::textBoxTextColourId,
                        juce::Colour::fromRGB (32, 32, 36));
        knob.setColour (juce::Slider::textBoxBackgroundColourId,
                        juce::Colours::transparentBlack);
        knob.setColour (juce::Slider::textBoxOutlineColourId,
                        juce::Colours::transparentBlack);

        addAndMakeVisible (knob);

        auto& label = labels[i];
        label.setText (parameterNames[i], juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setFont (juce::Font (juce::FontOptions (13.0f).withStyle ("Bold")));
        label.setColour (juce::Label::textColourId,
                         juce::Colour::fromRGB (35, 35, 40));
        addAndMakeVisible (label);

        attachments[i] = std::make_unique<Attachment> (
            processor.apvts,
            parameterIDs[i],
            knob);
    }
}

void AngelEngineAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour::fromRGB (249, 249, 250));

    auto area = getLocalBounds().toFloat();

    g.setColour (juce::Colour::fromRGB (24, 24, 28));
    g.setFont (juce::Font (juce::FontOptions (28.0f).withStyle ("Bold")));
    g.drawText ("ANGEL ENGINE",
                area.removeFromTop (58.0f),
                juce::Justification::centred,
                false);

    g.setColour (juce::Colour::fromRGB (125, 125, 135));
    g.setFont (juce::Font (juce::FontOptions (11.0f)));
    g.drawText ("melodic degradation / spectral space / unstable memory",
                0, 52, getWidth(), 22,
                juce::Justification::centred,
                false);

    g.setColour (juce::Colour::fromRGB (112, 76, 255).withAlpha (0.22f));

    for (int x = 40; x < getWidth(); x += 52)
    {
        const float y = 88.0f + 5.0f * std::sin (static_cast<float> (x) * 0.08f);
        g.fillEllipse (static_cast<float> (x), y, 3.0f, 3.0f);
    }
}

void AngelEngineAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (28);
    area.removeFromTop (76);

    const int gap = 8;
    const int cellWidth = (area.getWidth() - gap * 5) / 6;

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto cell = area.removeFromLeft (cellWidth);

        if (i + 1 < knobs.size())
            area.removeFromLeft (gap);

        labels[i].setBounds (cell.removeFromTop (28));
        knobs[i].setBounds (cell.reduced (2, 2));
    }
}
