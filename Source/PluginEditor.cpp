#include "PluginEditor.h"

namespace
{
    const juce::Colour panelBase  { 0xff6f6d5f };
    const juce::Colour panelDark  { 0xff35362f };
    const juce::Colour panelLine  { 0xff272822 };
    const juce::Colour ink        { 0xffe8dfc8 };
    const juce::Colour inkDim     { 0xffc7bea7 };
    const juce::Colour accent     { 0xffc6ae6d };

    void drawScrew (juce::Graphics& g, float x, float y)
    {
        g.setColour (juce::Colours::black.withAlpha (0.28f));
        g.fillEllipse (x - 4.0f, y - 3.0f, 9.0f, 9.0f);
        g.setColour (juce::Colour (0xffaaa58f));
        g.fillEllipse (x - 4.0f, y - 4.0f, 8.0f, 8.0f);
        g.setColour (juce::Colour (0xff45463f));
        g.drawLine (x - 2.2f, y, x + 2.2f, y, 1.1f);
    }
}

AngelEngineAudioProcessorEditor::AngelEngineAudioProcessorEditor (
    AngelEngineAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p)
{
    setLookAndFeel (&analogLook);
    setSize (980, 520);
    setResizable (true, true);
    setResizeLimits (820, 440, 1280, 700);

    mainButton.setClickingTogglesState (false);
    modButton.setClickingTogglesState (false);
    mainButton.onClick = [this] { showMainPage(); };
    modButton.onClick = [this] { showModPage(); };
    addAndMakeVisible (mainButton);
    addAndMakeVisible (modButton);

    for (size_t i = 0; i < mainKnobs.size(); ++i)
    {
        styleKnob (mainKnobs[i]);
        styleLabel (mainLabels[i], mainParameterNames[i], i == 4 ? 13.5f : 12.0f);
        addAndMakeVisible (mainKnobs[i]);
        addAndMakeVisible (mainLabels[i]);

        mainAttachments[i] = std::make_unique<SliderAttachment> (
            audioProcessor.apvts, mainParameterIDs[i], mainKnobs[i]);
    }

    const juce::StringArray shapes { "SINE", "TRIANGLE", "SAW", "SQUARE", "RANDOM" };
    const juce::StringArray targets { "NONE", "AGE", "AIR", "GHOST", "WIDTH", "MELT", "CHAOS" };

    for (int i = 0; i < 3; ++i)
    {
        const auto n = juce::String (i + 1);

        styleKnob (rateKnobs[static_cast<size_t> (i)]);
        styleKnob (depthKnobs[static_cast<size_t> (i)]);

        styleLabel (lfoLabels[static_cast<size_t> (i)], "LFO 0" + n, 15.0f);
        styleLabel (rateLabels[static_cast<size_t> (i)], "RATE", 10.5f);
        styleLabel (depthLabels[static_cast<size_t> (i)], "DEPTH", 10.5f);
        styleLabel (shapeLabels[static_cast<size_t> (i)], "SHAPE", 9.5f);
        styleLabel (targetLabels[static_cast<size_t> (i)], "TARGET", 9.5f);

        auto& shape = shapeBoxes[static_cast<size_t> (i)];
        auto& target = targetBoxes[static_cast<size_t> (i)];

        for (int item = 0; item < shapes.size(); ++item)
            shape.addItem (shapes[item], item + 1);

        for (int item = 0; item < targets.size(); ++item)
            target.addItem (targets[item], item + 1);

        addAndMakeVisible (rateKnobs[static_cast<size_t> (i)]);
        addAndMakeVisible (depthKnobs[static_cast<size_t> (i)]);
        addAndMakeVisible (lfoLabels[static_cast<size_t> (i)]);
        addAndMakeVisible (rateLabels[static_cast<size_t> (i)]);
        addAndMakeVisible (depthLabels[static_cast<size_t> (i)]);
        addAndMakeVisible (shapeLabels[static_cast<size_t> (i)]);
        addAndMakeVisible (targetLabels[static_cast<size_t> (i)]);
        addAndMakeVisible (shape);
        addAndMakeVisible (target);

        rateAttachments[static_cast<size_t> (i)] = std::make_unique<SliderAttachment> (
            audioProcessor.apvts, "lfo" + n + "Rate", rateKnobs[static_cast<size_t> (i)]);

        depthAttachments[static_cast<size_t> (i)] = std::make_unique<SliderAttachment> (
            audioProcessor.apvts, "lfo" + n + "Depth", depthKnobs[static_cast<size_t> (i)]);

        shapeAttachments[static_cast<size_t> (i)] = std::make_unique<ComboAttachment> (
            audioProcessor.apvts, "lfo" + n + "Shape", shape);

        targetAttachments[static_cast<size_t> (i)] = std::make_unique<ComboAttachment> (
            audioProcessor.apvts, "lfo" + n + "Dest", target);
    }

    updatePageVisibility();
}

AngelEngineAudioProcessorEditor::~AngelEngineAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void AngelEngineAudioProcessorEditor::styleKnob (juce::Slider& slider)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setMouseDragSensitivity (180);
}

void AngelEngineAudioProcessorEditor::styleLabel (juce::Label& label,
                                                   const juce::String& text,
                                                   float size)
{
    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::Font (juce::FontOptions (size).withStyle ("Bold")));
    label.setColour (juce::Label::textColourId, ink);
}

void AngelEngineAudioProcessorEditor::showMainPage()
{
    modPageVisible = false;
    updatePageVisibility();
    repaint();
}

void AngelEngineAudioProcessorEditor::showModPage()
{
    modPageVisible = true;
    updatePageVisibility();
    repaint();
}

void AngelEngineAudioProcessorEditor::updatePageVisibility()
{
    mainButton.setToggleState (! modPageVisible, juce::dontSendNotification);
    modButton.setToggleState (modPageVisible, juce::dontSendNotification);

    for (auto& control : mainKnobs) control.setVisible (! modPageVisible);
    for (auto& label : mainLabels) label.setVisible (! modPageVisible);

    for (int i = 0; i < 3; ++i)
    {
        const bool visible = modPageVisible;
        rateKnobs[static_cast<size_t> (i)].setVisible (visible);
        depthKnobs[static_cast<size_t> (i)].setVisible (visible);
        lfoLabels[static_cast<size_t> (i)].setVisible (visible);
        rateLabels[static_cast<size_t> (i)].setVisible (visible);
        depthLabels[static_cast<size_t> (i)].setVisible (visible);
        shapeLabels[static_cast<size_t> (i)].setVisible (visible);
        targetLabels[static_cast<size_t> (i)].setVisible (visible);
        shapeBoxes[static_cast<size_t> (i)].setVisible (visible);
        targetBoxes[static_cast<size_t> (i)].setVisible (visible);
    }
}

void AngelEngineAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (panelBase);

    // Top fascia.
    g.setColour (panelDark);
    g.fillRect (0, 0, getWidth(), 78);

    // Slightly uneven seams to evoke an old painted metal panel.
    g.setColour (juce::Colours::black.withAlpha (0.16f));
    g.drawLine (0.0f, 78.0f, static_cast<float> (getWidth()), 78.0f, 2.0f);
    g.drawLine (0.0f, static_cast<float> (getHeight() - 25),
                static_cast<float> (getWidth()), static_cast<float> (getHeight() - 25), 1.0f);

    g.setColour (ink);
    g.setFont (juce::Font (juce::FontOptions (25.0f).withStyle ("Bold")));
    g.drawText ("ANGEL ENGINE", 34, 14, 280, 32, juce::Justification::centredLeft, false);

    g.setColour (inkDim);
    g.setFont (juce::Font (juce::FontOptions (10.5f).withStyle ("Bold")));
    g.drawText ("ANALOG INSTABILITY PROCESSOR  /  AE-01",
                36, 45, 320, 18, juce::Justification::centredLeft, false);

    drawScrew (g, 16.0f, 16.0f);
    drawScrew (g, static_cast<float> (getWidth() - 16), 16.0f);
    drawScrew (g, 16.0f, static_cast<float> (getHeight() - 14));
    drawScrew (g, static_cast<float> (getWidth() - 16), static_cast<float> (getHeight() - 14));

    auto content = getLocalBounds().reduced (28);
    content.removeFromTop (72);
    content.removeFromBottom (20);

    if (! modPageVisible)
    {
        const int gap = 12;
        const int usable = content.getWidth();
        const int groupWidth = (usable - gap * 3) / 4;

        for (int group = 0; group < 4; ++group)
        {
            auto x = content.getX() + group * (groupWidth + gap);
            auto box = juce::Rectangle<int> (x, content.getY(), groupWidth, content.getHeight());

            g.setColour (panelDark.withAlpha (group == 3 ? 0.72f : 0.38f));
            g.fillRoundedRectangle (box.toFloat(), 3.0f);
            g.setColour (panelLine.withAlpha (0.82f));
            g.drawRoundedRectangle (box.toFloat(), 3.0f, 1.0f);
        }

        g.setColour (accent);
        g.setFont (juce::Font (juce::FontOptions (9.5f).withStyle ("Bold")));
        g.drawText ("TONE / WEAR", content.getX(), content.getY() + 11, groupWidth, 18,
                    juce::Justification::centred, false);
        g.drawText ("MEMORY / SPACE", content.getX() + groupWidth + gap, content.getY() + 11, groupWidth, 18,
                    juce::Justification::centred, false);
        g.drawText ("IMAGE / MOTION", content.getX() + 2 * (groupWidth + gap), content.getY() + 11, groupWidth, 18,
                    juce::Justification::centred, false);
        g.drawText ("RETURN", content.getX() + 3 * (groupWidth + gap), content.getY() + 11, groupWidth, 18,
                    juce::Justification::centred, false);
    }
    else
    {
        const int gap = 16;
        const int stripWidth = (content.getWidth() - gap * 2) / 3;

        for (int i = 0; i < 3; ++i)
        {
            auto strip = juce::Rectangle<int> (content.getX() + i * (stripWidth + gap),
                                               content.getY(), stripWidth, content.getHeight());
            g.setColour (panelDark.withAlpha (0.54f));
            g.fillRoundedRectangle (strip.toFloat(), 3.0f);
            g.setColour (panelLine);
            g.drawRoundedRectangle (strip.toFloat(), 3.0f, 1.0f);

            g.setColour (accent.withAlpha (0.72f));
            g.fillRect (strip.getX() + 12, strip.getY() + 47, strip.getWidth() - 24, 2);
        }

        g.setColour (inkDim);
        g.setFont (juce::Font (juce::FontOptions (9.5f)));
        g.drawText ("FREE-RUN MODULATION  /  DEPTH = +/- MACRO RANGE",
                    content.getX(), content.getBottom() - 20, content.getWidth(), 16,
                    juce::Justification::centred, false);
    }

    g.setColour (inkDim.withAlpha (0.70f));
    g.setFont (juce::Font (juce::FontOptions (9.0f)));
    g.drawText ("v0.2", getWidth() - 66, getHeight() - 23, 42, 14,
                juce::Justification::centredRight, false);
}

void AngelEngineAudioProcessorEditor::resized()
{
    mainButton.setBounds (getWidth() - 190, 22, 68, 32);
    modButton.setBounds  (getWidth() - 112, 22, 68, 32);

    auto content = getLocalBounds().reduced (28);
    content.removeFromTop (72);
    content.removeFromBottom (20);

    if (! modPageVisible)
    {
        const int gap = 12;
        const int groupWidth = (content.getWidth() - gap * 3) / 4;

        std::array<juce::Rectangle<int>, 4> groups;
        for (int group = 0; group < 4; ++group)
            groups[static_cast<size_t> (group)] = juce::Rectangle<int> (
                content.getX() + group * (groupWidth + gap), content.getY(), groupWidth, content.getHeight());

        const std::array<std::array<int, 2>, 3> groupMap {{ {{0, 1}}, {{4, 2}}, {{3, 5}} }};

        for (int group = 0; group < 3; ++group)
        {
            auto inner = groups[static_cast<size_t> (group)].reduced (10, 36);
            const int half = inner.getWidth() / 2;

            for (int slot = 0; slot < 2; ++slot)
            {
                const int knobIndex = groupMap[static_cast<size_t> (group)][static_cast<size_t> (slot)];
                auto cell = juce::Rectangle<int> (inner.getX() + slot * half, inner.getY(), half, inner.getHeight());
                auto labelArea = cell.removeFromBottom (30);
                mainLabels[static_cast<size_t> (knobIndex)].setBounds (labelArea);

                if (knobIndex == 4)
                    mainKnobs[static_cast<size_t> (knobIndex)].setBounds (cell.expanded (5, 8));
                else
                    mainKnobs[static_cast<size_t> (knobIndex)].setBounds (cell.reduced (1, 8));
            }
        }

        auto mixInner = groups[3].reduced (18, 44);
        mainLabels[6].setBounds (mixInner.removeFromBottom (30));
        mainKnobs[6].setBounds (mixInner.reduced (2, 22));
    }
    else
    {
        const int gap = 16;
        const int stripWidth = (content.getWidth() - gap * 2) / 3;

        for (int i = 0; i < 3; ++i)
        {
            auto strip = juce::Rectangle<int> (content.getX() + i * (stripWidth + gap),
                                               content.getY(), stripWidth, content.getHeight()).reduced (14);

            lfoLabels[static_cast<size_t> (i)].setBounds (strip.removeFromTop (34));
            strip.removeFromTop (14);

            auto knobsArea = strip.removeFromTop (190);
            const int half = knobsArea.getWidth() / 2;

            auto rateArea = knobsArea.removeFromLeft (half).reduced (4, 0);
            auto depthArea = knobsArea.reduced (4, 0);

            rateLabels[static_cast<size_t> (i)].setBounds (rateArea.removeFromBottom (24));
            depthLabels[static_cast<size_t> (i)].setBounds (depthArea.removeFromBottom (24));
            rateKnobs[static_cast<size_t> (i)].setBounds (rateArea);
            depthKnobs[static_cast<size_t> (i)].setBounds (depthArea);

            strip.removeFromTop (8);

            auto shapeRow = strip.removeFromTop (56);
            shapeLabels[static_cast<size_t> (i)].setBounds (shapeRow.removeFromTop (18));
            shapeBoxes[static_cast<size_t> (i)].setBounds (shapeRow.reduced (14, 1));

            strip.removeFromTop (6);

            auto targetRow = strip.removeFromTop (56);
            targetLabels[static_cast<size_t> (i)].setBounds (targetRow.removeFromTop (18));
            targetBoxes[static_cast<size_t> (i)].setBounds (targetRow.reduced (14, 1));
        }
    }
}
