#include "PluginEditor.h"
#include <algorithm>

namespace
{
    const juce::Colour shell      { 0xffb9ad96 };
    const juce::Colour face       { 0xffd6cbb6 };
    const juce::Colour faceDark   { 0xffa89c86 };
    const juce::Colour header     { 0xff4b5046 };
    const juce::Colour ink        { 0xff181916 };

    const std::array<juce::Colour, 6> effectColours {
        juce::Colour (0xffbecda8), juce::Colour (0xffd99278), juce::Colour (0xffdfbf52),
        juce::Colour (0xff93b5c9), juce::Colour (0xffb29abf), juce::Colour (0xff9dbda7)
    };

    const std::array<juce::Colour, 3> lfoColours {
        juce::Colour (0xff93b5c9), juce::Colour (0xffb29abf), juce::Colour (0xff9dbda7)
    };

    void drawScrew (juce::Graphics& g, float x, float y, float size = 8.0f)
    {
        g.setColour (juce::Colours::black.withAlpha (0.28f));
        g.fillEllipse (x - size * 0.5f + 1.0f, y - size * 0.5f + 1.5f, size, size);
        g.setColour (juce::Colour (0xff535047));
        g.fillEllipse (x - size * 0.5f, y - size * 0.5f, size, size);
        g.setColour (juce::Colour (0xffaaa18e));
        g.drawLine (x - size * 0.24f, y, x + size * 0.24f, y, 1.0f);
    }

    void drawWear (juce::Graphics& g, juce::Rectangle<int> bounds, int seed)
    {
        g.setColour (juce::Colour (0xff5a5144).withAlpha (0.09f));
        for (int i = 0; i < 26; ++i)
        {
            const int x = bounds.getX() + ((i * 97 + seed * 53) % juce::jmax (1, bounds.getWidth()));
            const int y = bounds.getY() + ((i * 61 + seed * 29) % juce::jmax (1, bounds.getHeight()));
            const int w = 2 + ((i * 7 + seed) % 9);
            g.drawLine (static_cast<float> (x), static_cast<float> (y),
                        static_cast<float> (x + w), static_cast<float> (y + ((i % 3) - 1)), 0.7f);
        }
    }

    void drawPanel (juce::Graphics& g, juce::Rectangle<int> bounds, juce::Colour strip,
                    const juce::String& title, const juce::String& subtitle, int seed)
    {
        auto r = bounds.toFloat();
        g.setColour (face);
        g.fillRect (r);
        g.setColour (juce::Colour (0xff6f6658));
        g.drawRect (r, 1.0f);

        auto band = bounds.removeFromTop (30);
        g.setColour (strip);
        g.fillRect (band);
        g.setColour (strip.darker (0.25f));
        g.drawLine (static_cast<float> (band.getX()), static_cast<float> (band.getBottom()),
                    static_cast<float> (band.getRight()), static_cast<float> (band.getBottom()), 1.0f);

        g.setColour (ink);
        g.setFont (juce::Font (juce::FontOptions (13.5f).withStyle ("Bold")));
        g.drawText (title, band.reduced (10, 0), juce::Justification::centredLeft, false);
        g.setFont (juce::Font (juce::FontOptions (7.0f).withStyle ("Bold")));
        g.drawText (subtitle, band.reduced (10, 0), juce::Justification::centredRight, false);

        drawWear (g, bounds, seed);
        drawScrew (g, static_cast<float> (r.getX() + 7.0f), static_cast<float> (r.getBottom() - 7.0f), 6.0f);
        drawScrew (g, static_cast<float> (r.getRight() - 7.0f), static_cast<float> (r.getBottom() - 7.0f), 6.0f);
    }
}

IngeniumAudioProcessorEditor::IngeniumAudioProcessorEditor (IngeniumAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&ingeniumLook);
    setSize (1240, 820);
    setResizable (true, true);
    setResizeLimits (1080, 720, 1580, 1040);

    mainButton.setClickingTogglesState (false);
    modButton.setClickingTogglesState (false);
    mainButton.onClick = [this] { showMainPage(); };
    modButton.onClick = [this] { showModPage(); };
    addAndMakeVisible (mainButton);
    addAndMakeVisible (modButton);

    for (size_t i = 0; i < mainKnobs.size(); ++i)
    {
        styleKnob (mainKnobs[i], false);
        styleLabel (mainLabels[i], mainParameterNames[i], i < 6 ? 10.0f : 9.0f);
        addAndMakeVisible (mainKnobs[i]);
        addAndMakeVisible (mainLabels[i]);
        mainAttachments[i] = std::make_unique<SliderAttachment> (audioProcessor.apvts, mainParameterIDs[i], mainKnobs[i]);
    }

    for (size_t i = 0; i < subKnobs.size(); ++i)
    {
        styleKnob (subKnobs[i], true);
        styleLabel (subLabels[i], subParameterNames[i], 7.5f);
        addAndMakeVisible (subKnobs[i]);
        addAndMakeVisible (subLabels[i]);
        subAttachments[i] = std::make_unique<SliderAttachment> (audioProcessor.apvts, subParameterIDs[i], subKnobs[i]);
    }

    grainCaptureBox.addItemList (juce::StringArray { "FLOW", "TRANSIENT", "GRID" }, 1);
    ghostDivisionBox.addItemList (juce::StringArray { "1/4", "1/8", "1/8D", "1/8T", "1/16" }, 1);
    styleCombo (grainCaptureBox);
    styleCombo (ghostDivisionBox);
    styleLabel (grainCaptureLabel, "CAPTURE", 7.5f);
    styleLabel (ghostDivisionLabel, "DIVISION", 7.5f);
    addAndMakeVisible (grainCaptureBox);
    addAndMakeVisible (grainCaptureLabel);
    addAndMakeVisible (ghostDivisionBox);
    addAndMakeVisible (ghostDivisionLabel);
    grainCaptureAttachment = std::make_unique<ComboAttachment> (audioProcessor.apvts, "grainCapture", grainCaptureBox);
    ghostDivisionAttachment = std::make_unique<ComboAttachment> (audioProcessor.apvts, "ghostDivision", ghostDivisionBox);

    const juce::StringArray shapes { "SINE", "TRIANGLE", "SAW", "SQUARE", "RANDOM" };
    const juce::StringArray targets { "NONE", "AGE", "MELT", "GRAIN", "GHOST", "SMEAR", "CHAOS" };
    const juce::StringArray divisions { "1/1", "1/2", "1/4", "1/8", "1/8D", "1/8T", "1/16", "1/32" };

    for (int i = 0; i < 3; ++i)
    {
        const auto n = juce::String (i + 1);
        styleKnob (rateKnobs[i], false);
        styleKnob (depthKnobs[i], false);
        styleLabel (lfoLabels[i], "LFO 0" + n, 12.0f);
        styleLabel (rateLabels[i], "RATE", 8.0f);
        styleLabel (depthLabels[i], "DEPTH", 8.0f);
        styleLabel (shapeLabels[i], "SHAPE", 7.2f);
        styleLabel (targetLabels[i], "TARGET", 7.2f);
        styleLabel (divisionLabels[i], "DIVISION", 7.2f);

        shapeBoxes[i].addItemList (shapes, 1);
        targetBoxes[i].addItemList (targets, 1);
        divisionBoxes[i].addItemList (divisions, 1);
        styleCombo (shapeBoxes[i]);
        styleCombo (targetBoxes[i]);
        styleCombo (divisionBoxes[i]);
        syncButtons[i].setButtonText ("SYNC");
        syncButtons[i].setClickingTogglesState (true);

        addAndMakeVisible (rateKnobs[i]);
        addAndMakeVisible (depthKnobs[i]);
        addAndMakeVisible (lfoLabels[i]);
        addAndMakeVisible (rateLabels[i]);
        addAndMakeVisible (depthLabels[i]);
        addAndMakeVisible (shapeLabels[i]);
        addAndMakeVisible (targetLabels[i]);
        addAndMakeVisible (divisionLabels[i]);
        addAndMakeVisible (shapeBoxes[i]);
        addAndMakeVisible (targetBoxes[i]);
        addAndMakeVisible (divisionBoxes[i]);
        addAndMakeVisible (syncButtons[i]);

        rateAttachments[i] = std::make_unique<SliderAttachment> (audioProcessor.apvts, "lfo" + n + "Rate", rateKnobs[i]);
        depthAttachments[i] = std::make_unique<SliderAttachment> (audioProcessor.apvts, "lfo" + n + "Depth", depthKnobs[i]);
        shapeAttachments[i] = std::make_unique<ComboAttachment> (audioProcessor.apvts, "lfo" + n + "Shape", shapeBoxes[i]);
        targetAttachments[i] = std::make_unique<ComboAttachment> (audioProcessor.apvts, "lfo" + n + "Dest", targetBoxes[i]);
        divisionAttachments[i] = std::make_unique<ComboAttachment> (audioProcessor.apvts, "lfo" + n + "Division", divisionBoxes[i]);
        syncAttachments[i] = std::make_unique<ButtonAttachment> (audioProcessor.apvts, "lfo" + n + "Sync", syncButtons[i]);
    }

    updatePageVisibility();
}

IngeniumAudioProcessorEditor::~IngeniumAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void IngeniumAudioProcessorEditor::styleKnob (juce::Slider& slider, bool small)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setMouseDragSensitivity (small ? 150 : 190);
}

void IngeniumAudioProcessorEditor::styleLabel (juce::Label& label, const juce::String& text, float size)
{
    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::Font (juce::FontOptions (size).withStyle ("Bold")));
    label.setColour (juce::Label::textColourId, ink);
}

void IngeniumAudioProcessorEditor::styleCombo (juce::ComboBox& box)
{
    box.setJustificationType (juce::Justification::centredLeft);
}

void IngeniumAudioProcessorEditor::showMainPage()
{
    modPageVisible = false;
    updatePageVisibility();
    resized();
    repaint();
}

void IngeniumAudioProcessorEditor::showModPage()
{
    modPageVisible = true;
    updatePageVisibility();
    resized();
    repaint();
}

void IngeniumAudioProcessorEditor::updatePageVisibility()
{
    mainButton.setToggleState (! modPageVisible, juce::dontSendNotification);
    modButton.setToggleState (modPageVisible, juce::dontSendNotification);

    for (auto& c : mainKnobs) c.setVisible (! modPageVisible);
    for (auto& c : mainLabels) c.setVisible (! modPageVisible);
    for (auto& c : subKnobs) c.setVisible (! modPageVisible);
    for (auto& c : subLabels) c.setVisible (! modPageVisible);
    grainCaptureBox.setVisible (! modPageVisible);
    grainCaptureLabel.setVisible (! modPageVisible);
    ghostDivisionBox.setVisible (! modPageVisible);
    ghostDivisionLabel.setVisible (! modPageVisible);

    for (int i = 0; i < 3; ++i)
    {
        const bool visible = modPageVisible;
        rateKnobs[i].setVisible (visible);
        depthKnobs[i].setVisible (visible);
        lfoLabels[i].setVisible (visible);
        rateLabels[i].setVisible (visible);
        depthLabels[i].setVisible (visible);
        shapeLabels[i].setVisible (visible);
        targetLabels[i].setVisible (visible);
        divisionLabels[i].setVisible (visible);
        shapeBoxes[i].setVisible (visible);
        targetBoxes[i].setVisible (visible);
        divisionBoxes[i].setVisible (visible);
        syncButtons[i].setVisible (visible);
    }
}

void IngeniumAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (shell);
    g.setColour (juce::Colour (0xff5f574a));
    g.drawRect (getLocalBounds().toFloat().reduced (1.0f), 2.0f);

    auto headerArea = getLocalBounds().reduced (24);
    headerArea.setHeight (74);
    g.setColour (header);
    g.fillRect (headerArea);
    g.setColour (juce::Colour (0xff30342e));
    g.drawRect (headerArea, 1);

    drawScrew (g, 14.0f, 14.0f);
    drawScrew (g, static_cast<float> (getWidth() - 14), 14.0f);
    drawScrew (g, 14.0f, static_cast<float> (getHeight() - 14));
    drawScrew (g, static_cast<float> (getWidth() - 14), static_cast<float> (getHeight() - 14));

    g.setColour (juce::Colour (0xffece4d2));
    g.setFont (juce::Font (juce::FontOptions (29.0f).withStyle ("Bold")));
    g.drawText ("INGENIUM", headerArea.getX() + 20, headerArea.getY() + 8, 300, 36, juce::Justification::centredLeft, false);
    g.setFont (juce::Font (juce::FontOptions (8.6f).withStyle ("Bold")));
    g.drawText ("ANALOG INSTABILITY / MEMORY PROCESSOR", headerArea.getX() + 22, headerArea.getY() + 43, 360, 18,
                juce::Justification::centredLeft, false);
    g.setColour (juce::Colour (0xffcbc1ad));
    g.drawText ("IG-01  /  REV.07", headerArea.getRight() - 310, headerArea.getY() + 17, 150, 18,
                juce::Justification::centredRight, false);

    auto content = getLocalBounds().reduced (30);
    content.removeFromTop (82);

    if (! modPageVisible)
    {
        auto output = content.removeFromBottom (135);
        content.removeFromBottom (10);
        const int gap = 10;
        const int colW = (content.getWidth() - gap * 2) / 3;
        const int rowH = (content.getHeight() - gap) / 2;

        std::array<juce::Rectangle<int>, 6> panels;
        panels[0] = { content.getX(), content.getY(), colW, rowH };
        panels[1] = { content.getX() + colW + gap, content.getY(), colW, rowH };
        panels[2] = { content.getX() + (colW + gap) * 2, content.getY(), colW, rowH };
        panels[3] = { content.getX(), content.getY() + rowH + gap, colW, rowH };
        panels[4] = { content.getX() + colW + gap, content.getY() + rowH + gap, colW, rowH };
        panels[5] = { content.getX() + (colW + gap) * 2, content.getY() + rowH + gap, colW, rowH };

        const juce::StringArray subtitles { "TAPE / WEAR", "LIQUID MOTION", "GRANULAR MEMORY",
                                             "MULTI-HEAD ECHO", "DIFFUSION / TEXTURE", "STOCHASTIC" };
        for (int i = 0; i < 6; ++i)
            drawPanel (g, panels[i], effectColours[i], mainParameterNames[i], subtitles[i], 20 + i * 13);

        g.setColour (faceDark);
        g.fillRect (output);
        g.setColour (juce::Colour (0xff6f6658));
        g.drawRect (output, 1);
        g.setColour (ink);
        g.setFont (juce::Font (juce::FontOptions (9.0f).withStyle ("Bold")));
        g.drawText ("MASTER / OUTPUT", output.reduced (12).removeFromTop (18), juce::Justification::centredLeft, false);
        drawWear (g, output, 121);
        const int third = output.getWidth() / 3;
        g.setColour (juce::Colour (0xff7f7463));
        g.drawVerticalLine (output.getX() + third, static_cast<float> (output.getY() + 24), static_cast<float> (output.getBottom() - 10));
        g.drawVerticalLine (output.getX() + third * 2, static_cast<float> (output.getY() + 24), static_cast<float> (output.getBottom() - 10));
    }
    else
    {
        const int gap = 12;
        const int colW = (content.getWidth() - gap * 2) / 3;
        for (int i = 0; i < 3; ++i)
        {
            auto r = juce::Rectangle<int> (content.getX() + i * (colW + gap), content.getY(), colW, content.getHeight() - 54);
            drawPanel (g, r, lfoColours[i], "LFO 0" + juce::String (i + 1), "MODULATION GENERATOR", 170 + i * 17);
        }

        auto footer = juce::Rectangle<int> (content.getX(), content.getBottom() - 44, content.getWidth(), 44);
        g.setColour (header.darker (0.04f));
        g.fillRect (footer);
        g.setColour (juce::Colour (0xffd8cfbd));
        g.setFont (juce::Font (juce::FontOptions (8.0f).withStyle ("Bold")));
        g.drawText ("MOD BUS  /  FREE OR HOST-SYNCED  /  DESTINATIONS: AGE · MELT · GRAIN · GHOST · SMEAR · CHAOS",
                    footer.reduced (12), juce::Justification::centredLeft, false);
    }
}

void IngeniumAudioProcessorEditor::layoutModule (juce::Rectangle<int> bounds,
                                                 int macroIndex,
                                                 std::initializer_list<int> subIndices,
                                                 juce::ComboBox* optionalBox,
                                                 juce::Label* optionalLabel)
{
    bounds.removeFromTop (34);
    const int macroW = juce::jmin (138, static_cast<int> (bounds.getWidth() * 0.38f));
    auto macroArea = bounds.removeFromLeft (macroW);
    macroArea.reduce (6, 8);
    mainKnobs[macroIndex].setBounds (macroArea.getX(), macroArea.getY() + 4, macroArea.getWidth(), macroArea.getHeight() - 26);
    mainLabels[macroIndex].setBounds (macroArea.getX(), macroArea.getBottom() - 23, macroArea.getWidth(), 18);

    auto subArea = bounds.reduced (4, 7);
    if (optionalBox != nullptr && optionalLabel != nullptr)
    {
        auto comboRow = subArea.removeFromBottom (42);
        optionalLabel->setBounds (comboRow.removeFromTop (13));
        optionalBox->setBounds (comboRow.reduced (2, 1));
        subArea.removeFromBottom (2);
    }

    std::vector<int> ids (subIndices);
    const int count = static_cast<int> (ids.size());
    const int cols = count > 4 ? 3 : 2;
    const int rows = juce::jmax (1, (count + cols - 1) / cols);
    const int cellW = subArea.getWidth() / cols;
    const int cellH = subArea.getHeight() / rows;

    for (int n = 0; n < count; ++n)
    {
        const int col = n % cols;
        const int row = n / cols;
        auto cell = juce::Rectangle<int> (subArea.getX() + col * cellW, subArea.getY() + row * cellH, cellW, cellH).reduced (2);
        const int idx = ids[n];
        subKnobs[idx].setBounds (cell.getX(), cell.getY(), cell.getWidth(), juce::jmax (32, cell.getHeight() - 14));
        subLabels[idx].setBounds (cell.getX(), cell.getBottom() - 16, cell.getWidth(), 14);
    }
}

void IngeniumAudioProcessorEditor::resized()
{
    auto headerArea = getLocalBounds().reduced (24);
    headerArea.setHeight (74);
    mainButton.setBounds (headerArea.getRight() - 150, headerArea.getY() + 17, 64, 32);
    modButton.setBounds  (headerArea.getRight() - 78, headerArea.getY() + 17, 58, 32);

    auto content = getLocalBounds().reduced (30);
    content.removeFromTop (82);

    if (! modPageVisible)
    {
        auto output = content.removeFromBottom (135);
        content.removeFromBottom (10);
        const int gap = 10;
        const int colW = (content.getWidth() - gap * 2) / 3;
        const int rowH = (content.getHeight() - gap) / 2;

        std::array<juce::Rectangle<int>, 6> p;
        p[0] = { content.getX(), content.getY(), colW, rowH };
        p[1] = { content.getX() + colW + gap, content.getY(), colW, rowH };
        p[2] = { content.getX() + (colW + gap) * 2, content.getY(), colW, rowH };
        p[3] = { content.getX(), content.getY() + rowH + gap, colW, rowH };
        p[4] = { content.getX() + colW + gap, content.getY() + rowH + gap, colW, rowH };
        p[5] = { content.getX() + (colW + gap) * 2, content.getY() + rowH + gap, colW, rowH };

        layoutModule (p[0], 0, { 0, 1, 2 });
        layoutModule (p[1], 1, { 3, 4, 5 });
        layoutModule (p[2], 2, { 6, 7, 8, 9, 10, 11 }, &grainCaptureBox, &grainCaptureLabel);
        layoutModule (p[3], 3, { 12, 13, 14 }, &ghostDivisionBox, &ghostDivisionLabel);
        layoutModule (p[4], 4, { 15, 16, 17, 18 });
        layoutModule (p[5], 5, { 19, 20 });

        auto body = output.reduced (10);
        body.removeFromTop (20);
        const int third = body.getWidth() / 3;
        auto airArea = body.removeFromLeft (third).reduced (8, 2);
        auto widthArea = body.removeFromLeft (third).reduced (8, 2);
        auto mixArea = body.reduced (8, 2);

        auto layoutOutput = [this] (juce::Rectangle<int> area, int macroIndex, std::initializer_list<int> subs)
        {
            auto big = area.removeFromLeft (juce::jmin (112, area.getWidth() / 2));
            mainKnobs[macroIndex].setBounds (big.getX(), big.getY(), big.getWidth(), big.getHeight() - 16);
            mainLabels[macroIndex].setBounds (big.getX(), big.getBottom() - 18, big.getWidth(), 16);
            std::vector<int> ids (subs);
            const int count = juce::jmax (1, static_cast<int> (ids.size()));
            const int w = area.getWidth() / count;
            for (int i = 0; i < static_cast<int> (ids.size()); ++i)
            {
                auto cell = juce::Rectangle<int> (area.getX() + i * w, area.getY(), w, area.getHeight()).reduced (2);
                subKnobs[ids[i]].setBounds (cell.getX(), cell.getY(), cell.getWidth(), cell.getHeight() - 16);
                subLabels[ids[i]].setBounds (cell.getX(), cell.getBottom() - 16, cell.getWidth(), 14);
            }
        };

        layoutOutput (airArea, 6, { 21, 22 });
        layoutOutput (widthArea, 7, { 23 });
        mainKnobs[8].setBounds (mixArea.getX() + mixArea.getWidth() / 4, mixArea.getY(), mixArea.getWidth() / 2, mixArea.getHeight() - 16);
        mainLabels[8].setBounds (mixArea.getX(), mixArea.getBottom() - 18, mixArea.getWidth(), 16);
    }
    else
    {
        const int gap = 12;
        const int colW = (content.getWidth() - gap * 2) / 3;
        for (int i = 0; i < 3; ++i)
        {
            auto r = juce::Rectangle<int> (content.getX() + i * (colW + gap), content.getY(), colW, content.getHeight() - 54);
            r.removeFromTop (38);
            auto knobRow = r.removeFromTop (190).reduced (12, 4);
            auto left = knobRow.removeFromLeft (knobRow.getWidth() / 2);
            rateKnobs[i].setBounds (left.reduced (10, 0));
            rateLabels[i].setBounds (left.getX(), left.getBottom() - 22, left.getWidth(), 18);
            depthKnobs[i].setBounds (knobRow.reduced (10, 0));
            depthLabels[i].setBounds (knobRow.getX(), knobRow.getBottom() - 22, knobRow.getWidth(), 18);
            lfoLabels[i].setBounds (r.getX(), r.getY() - 224, r.getWidth(), 20);

            auto row1 = r.removeFromTop (58).reduced (10, 3);
            shapeLabels[i].setBounds (row1.removeFromTop (14));
            shapeBoxes[i].setBounds (row1);
            auto row2 = r.removeFromTop (58).reduced (10, 3);
            targetLabels[i].setBounds (row2.removeFromTop (14));
            targetBoxes[i].setBounds (row2);
            auto row3 = r.removeFromTop (58).reduced (10, 3);
            divisionLabels[i].setBounds (row3.removeFromTop (14));
            divisionBoxes[i].setBounds (row3.removeFromLeft (juce::jmax (80, row3.getWidth() - 92)));
            row3.removeFromLeft (8);
            syncButtons[i].setBounds (row3);
        }
    }
}
