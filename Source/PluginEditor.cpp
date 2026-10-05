#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

namespace
{
    const juce::Colour shell      { 0xffc7bba4 };
    const juce::Colour face       { 0xffddd2bc };
    const juce::Colour faceDark   { 0xffaa9d86 };
    const juce::Colour header     { 0xff4a5047 };
    const juce::Colour ink        { 0xff191916 };
    const juce::Colour creamInk   { 0xffeee5d3 };

    const std::array<juce::Colour, 6> effectColours {
        juce::Colour (0xffcfdfc3),
        juce::Colour (0xffe9a58d),
        juce::Colour (0xffead06e),
        juce::Colour (0xffa9cadd),
        juce::Colour (0xffc8afd3),
        juce::Colour (0xffb8d4c0)
    };

    const std::array<juce::Colour, 3> lfoColours {
        juce::Colour (0xffa9cadd),
        juce::Colour (0xffc8afd3),
        juce::Colour (0xffb8d4c0)
    };

    void drawScrew (juce::Graphics& g, float x, float y, float size = 8.0f)
    {
        g.setColour (juce::Colours::black.withAlpha (0.30f));
        g.fillEllipse (x - size * 0.5f + 1.0f, y - size * 0.5f + 1.5f, size, size);
        g.setColour (juce::Colour (0xff4e4b43));
        g.fillEllipse (x - size * 0.5f, y - size * 0.5f, size, size);
        g.setColour (juce::Colour (0xffb5aa96));
        g.drawLine (x - size * 0.24f, y, x + size * 0.24f, y, 1.0f);
    }

    void drawWear (juce::Graphics& g, juce::Rectangle<int> bounds, int seed, float opacity = 0.08f)
    {
        g.setColour (juce::Colour (0xff554c40).withAlpha (opacity));
        for (int i = 0; i < 34; ++i)
        {
            const int x = bounds.getX() + ((i * 97 + seed * 53) % juce::jmax (1, bounds.getWidth()));
            const int y = bounds.getY() + ((i * 61 + seed * 29) % juce::jmax (1, bounds.getHeight()));
            const int w = 2 + ((i * 7 + seed) % 12);
            g.drawLine (static_cast<float> (x), static_cast<float> (y),
                        static_cast<float> (x + w), static_cast<float> (y + ((i % 3) - 1)), 0.65f);
        }
    }

    void drawMachinePanel (juce::Graphics& g,
                           juce::Rectangle<int> bounds,
                           juce::Colour colour,
                           const juce::String& title,
                           const juce::String& subtitle,
                           int seed)
    {
        auto r = bounds.toFloat();
        g.setColour (colour);
        g.fillRect (r);
        g.setColour (juce::Colour (0xff716858));
        g.drawRect (r, 1.0f);

        auto band = bounds.removeFromTop (31);
        g.setColour (colour.darker (0.07f));
        g.fillRect (band);
        g.setColour (colour.darker (0.26f));
        g.drawLine (static_cast<float> (band.getX()), static_cast<float> (band.getBottom()),
                    static_cast<float> (band.getRight()), static_cast<float> (band.getBottom()), 1.0f);

        g.setColour (ink);
        g.setFont (juce::Font (juce::FontOptions (14.0f).withStyle ("Bold")));
        g.drawText (title, band.reduced (9, 0), juce::Justification::centredLeft, false);
        g.setFont (juce::Font (juce::FontOptions (6.8f).withStyle ("Bold")));
        g.drawText (subtitle, band.reduced (8, 0), juce::Justification::centredRight, false);

        drawWear (g, bounds, seed, 0.065f);
        drawScrew (g, r.getX() + 7.0f, r.getBottom() - 7.0f, 6.0f);
        drawScrew (g, r.getRight() - 7.0f, r.getBottom() - 7.0f, 6.0f);
    }

    void drawVuMeter (juce::Graphics& g,
                      juce::Rectangle<int> bounds,
                      float level,
                      const juce::String& channel)
    {
        auto r = bounds.toFloat();
        g.setColour (juce::Colour (0xff171714));
        g.fillRect (r);
        g.setColour (juce::Colour (0xffe7c77c));
        g.fillRect (r.reduced (5.0f));
        g.setColour (juce::Colour (0xff493d2b));
        g.drawRect (r.reduced (5.0f), 1.0f);

        auto inner = r.reduced (11.0f);
        const auto pivot = juce::Point<float> (inner.getCentreX(), inner.getBottom() - 5.0f);
        const float radius = inner.getWidth() * 0.43f;

        g.setColour (juce::Colour (0xff2b241b));
        for (int i = 0; i <= 8; ++i)
        {
            const float a = -0.90f + static_cast<float> (i) / 8.0f * 1.80f;
            const float c = std::cos (a);
            const float s = std::sin (a);
            g.drawLine (pivot.x + c * radius * 0.72f,
                        pivot.y - s * radius * 0.72f,
                        pivot.x + c * radius * 0.84f,
                        pivot.y - s * radius * 0.84f,
                        i == 6 ? 1.4f : 0.8f);
        }

        const float db = 20.0f * std::log10 (juce::jmax (0.00001f, level));
        const float norm = juce::jmap (juce::jlimit (-30.0f, 3.0f, db), -30.0f, 3.0f, 0.0f, 1.0f);
        const float angle = -0.90f + norm * 1.80f;
        g.setColour (norm > 0.86f ? juce::Colour (0xff9b2f24) : juce::Colour (0xff3a2920));
        g.drawLine (pivot.x, pivot.y,
                    pivot.x + std::cos (angle) * radius * 0.72f,
                    pivot.y - std::sin (angle) * radius * 0.72f,
                    1.8f);

        g.setColour (ink);
        g.setFont (juce::Font (juce::FontOptions (8.0f).withStyle ("Bold")));
        g.drawText ("VU", bounds.getX(), bounds.getY() + 5, bounds.getWidth(), 13,
                    juce::Justification::centred, false);
        g.drawText (channel, bounds.getX(), bounds.getBottom() - 17, bounds.getWidth(), 12,
                    juce::Justification::centred, false);
    }
}

IngeniumAudioProcessorEditor::IngeniumAudioProcessorEditor (IngeniumAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&ingeniumLook);
    setSize (1460, 840);
    setResizable (true, true);
    setResizeLimits (1220, 720, 1820, 1080);

    mainButton.setClickingTogglesState (false);
    modButton.setClickingTogglesState (false);
    mainButton.onClick = [this] { showMainPage(); };
    modButton.onClick = [this] { showModPage(); };
    addAndMakeVisible (mainButton);
    addAndMakeVisible (modButton);

    bypassButton.setClickingTogglesState (true);
    addAndMakeVisible (bypassButton);
    bypassAttachment = std::make_unique<ButtonAttachment> (audioProcessor.apvts, "bypass", bypassButton);

    for (size_t i = 0; i < mainKnobs.size(); ++i)
    {
        styleKnob (mainKnobs[i], false);
        styleLabel (mainLabels[i], mainParameterNames[i], i < 6 ? 9.0f : 8.5f);
        addAndMakeVisible (mainKnobs[i]);
        addAndMakeVisible (mainLabels[i]);
        mainAttachments[i] = std::make_unique<SliderAttachment> (audioProcessor.apvts, mainParameterIDs[i], mainKnobs[i]);
    }

    for (size_t i = 0; i < subKnobs.size(); ++i)
    {
        styleKnob (subKnobs[i], true);
        styleLabel (subLabels[i], subParameterNames[i], 6.7f);
        addAndMakeVisible (subKnobs[i]);
        addAndMakeVisible (subLabels[i]);
        subAttachments[i] = std::make_unique<SliderAttachment> (audioProcessor.apvts, subParameterIDs[i], subKnobs[i]);
    }

    grainCaptureBox.addItemList (juce::StringArray { "FLOW", "TRANSIENT", "GRID" }, 1);
    ghostDivisionBox.addItemList (juce::StringArray { "1/4", "1/8", "1/8D", "1/8T", "1/16" }, 1);
    styleCombo (grainCaptureBox);
    styleCombo (ghostDivisionBox);
    styleLabel (grainCaptureLabel, "CAPTURE", 6.8f);
    styleLabel (ghostDivisionLabel, "DIVISION", 6.8f);
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
        styleLabel (lfoLabels[i], "LFO 0" + n, 11.0f);
        styleLabel (rateLabels[i], "RATE", 7.5f);
        styleLabel (depthLabels[i], "DEPTH", 7.5f);
        styleLabel (shapeLabels[i], "SHAPE", 6.8f);
        styleLabel (targetLabels[i], "TARGET", 6.8f);
        styleLabel (divisionLabels[i], "DIVISION", 6.8f);

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
    startTimerHz (30);
}

IngeniumAudioProcessorEditor::~IngeniumAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void IngeniumAudioProcessorEditor::timerCallback()
{
    repaint();
}

void IngeniumAudioProcessorEditor::styleKnob (juce::Slider& slider, bool small)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setMouseDragSensitivity (small ? 145 : 190);
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
    g.setColour (juce::Colour (0xff5d5549));
    g.drawRect (getLocalBounds().toFloat().reduced (1.0f), 2.0f);

    auto headerArea = getLocalBounds().reduced (22);
    headerArea.setHeight (76);
    g.setColour (header);
    g.fillRect (headerArea);
    g.setColour (juce::Colour (0xff2f332d));
    g.drawRect (headerArea, 1);

    drawScrew (g, 13.0f, 13.0f);
    drawScrew (g, static_cast<float> (getWidth() - 13), 13.0f);
    drawScrew (g, 13.0f, static_cast<float> (getHeight() - 13));
    drawScrew (g, static_cast<float> (getWidth() - 13), static_cast<float> (getHeight() - 13));

    g.setColour (creamInk);
    g.setFont (juce::Font (juce::FontOptions (30.0f).withStyle ("Bold")));
    g.drawText ("INGENIUM", headerArea.getX() + 18, headerArea.getY() + 7, 310, 37,
                juce::Justification::centredLeft, false);
    g.setFont (juce::Font (juce::FontOptions (8.2f).withStyle ("Bold")));
    g.drawText ("ANALOG INSTABILITY / MEMORY PROCESSOR", headerArea.getX() + 20, headerArea.getY() + 45, 390, 16,
                juce::Justification::centredLeft, false);
    g.setColour (juce::Colour (0xffcfc5b1));
    g.drawText ("IG-01  /  REV.08", headerArea.getRight() - 355, headerArea.getY() + 18, 140, 16,
                juce::Justification::centredRight, false);

    auto content = getLocalBounds().reduced (28);
    content.removeFromTop (84);

    if (! modPageVisible)
    {
        auto output = content.removeFromBottom (166);
        content.removeFromBottom (8);

        const int gap = 6;
        const int colW = (content.getWidth() - gap * 5) / 6;
        const juce::StringArray subtitles {
            "TAPE / WEAR", "LIQUID / ANALOG", "GRANULAR MEMORY",
            "MULTI-HEAD ECHO", "DIFFUSION", "STOCHASTIC"
        };

        for (int i = 0; i < 6; ++i)
        {
            auto r = juce::Rectangle<int> (content.getX() + i * (colW + gap), content.getY(), colW, content.getHeight());
            drawMachinePanel (g, r, effectColours[i], mainParameterNames[i], subtitles[i], 30 + i * 19);
        }

        g.setColour (faceDark);
        g.fillRect (output);
        g.setColour (juce::Colour (0xff716858));
        g.drawRect (output, 1);
        drawWear (g, output, 211, 0.07f);

        auto titleRow = output.reduced (12).removeFromTop (18);
        g.setColour (ink);
        g.setFont (juce::Font (juce::FontOptions (9.0f).withStyle ("Bold")));
        g.drawText ("OUTPUT / FINISH", titleRow, juce::Justification::centredLeft, false);

        auto meterArea = output.reduced (12);
        meterArea.removeFromTop (24);
        meterArea.setWidth (330);
        auto leftVu = meterArea.removeFromLeft (158).reduced (3, 6);
        meterArea.removeFromLeft (8);
        auto rightVu = meterArea.removeFromLeft (158).reduced (3, 6);
        drawVuMeter (g, leftVu, audioProcessor.getMeterLeft(), "L");
        drawVuMeter (g, rightVu, audioProcessor.getMeterRight(), "R");

        g.setColour (juce::Colour (0xff756a5a));
        g.drawVerticalLine (output.getX() + 352,
                            static_cast<float> (output.getY() + 26),
                            static_cast<float> (output.getBottom() - 10));
    }
    else
    {
        const int gap = 10;
        const int colW = (content.getWidth() - gap * 2) / 3;
        for (int i = 0; i < 3; ++i)
        {
            auto r = juce::Rectangle<int> (content.getX() + i * (colW + gap), content.getY(), colW, content.getHeight() - 48);
            drawMachinePanel (g, r, lfoColours[i], "LFO 0" + juce::String (i + 1), "DECLICKED MODULATOR", 190 + i * 17);

            g.setColour (ink.withAlpha (0.36f));
            auto wave = r.reduced (28);
            wave.removeFromTop (245);
            wave.setHeight (48);
            juce::Path p;
            const float mid = static_cast<float> (wave.getCentreY());
            p.startNewSubPath (static_cast<float> (wave.getX()), mid);
            for (int x = 1; x < wave.getWidth(); ++x)
            {
                const float phase = static_cast<float> (x) / static_cast<float> (juce::jmax (1, wave.getWidth())) * twoPi * 2.0f;
                p.lineTo (static_cast<float> (wave.getX() + x), mid - std::sin (phase) * 12.0f);
            }
            g.strokePath (p, juce::PathStrokeType (1.0f));
        }

        auto footer = juce::Rectangle<int> (content.getX(), content.getBottom() - 40, content.getWidth(), 40);
        g.setColour (header.darker (0.04f));
        g.fillRect (footer);
        g.setColour (juce::Colour (0xffddd4c2));
        g.setFont (juce::Font (juce::FontOptions (7.8f).withStyle ("Bold")));
        g.drawText ("MOD BUS  /  FREE OR HOST-SYNCED  /  SLEW-SAFE SAW & SQUARE  /  AGE · MELT · GRAIN · GHOST · SMEAR · CHAOS",
                    footer.reduced (12), juce::Justification::centredLeft, false);
    }
}

void IngeniumAudioProcessorEditor::layoutVerticalModule (juce::Rectangle<int> bounds,
                                                         int macroIndex,
                                                         std::initializer_list<int> subIndices,
                                                         juce::ComboBox* optionalBox,
                                                         juce::Label* optionalLabel)
{
    bounds.removeFromTop (35);
    bounds.reduce (5, 5);

    auto macroArea = bounds.removeFromTop (juce::jmin (190, static_cast<int> (bounds.getHeight() * 0.43f)));
    const int macroSize = juce::jmin (150, juce::jmin (macroArea.getWidth() - 8, macroArea.getHeight() - 20));
    mainKnobs[macroIndex].setBounds (macroArea.getCentreX() - macroSize / 2,
                                     macroArea.getY() + 2,
                                     macroSize,
                                     macroSize);
    mainLabels[macroIndex].setBounds (macroArea.getX(), macroArea.getBottom() - 18, macroArea.getWidth(), 16);

    if (optionalBox != nullptr && optionalLabel != nullptr)
    {
        auto comboRow = bounds.removeFromBottom (44);
        optionalLabel->setBounds (comboRow.removeFromTop (13));
        optionalBox->setBounds (comboRow.reduced (2, 2));
        bounds.removeFromBottom (2);
    }

    std::vector<int> ids (subIndices);
    const int count = static_cast<int> (ids.size());
    int cols = count;
    if (count == 4) cols = 2;
    if (count > 4) cols = 3;
    cols = juce::jmax (1, cols);
    const int rows = juce::jmax (1, (count + cols - 1) / cols);
    const int cellW = bounds.getWidth() / cols;
    const int cellH = bounds.getHeight() / rows;

    for (int n = 0; n < count; ++n)
    {
        const int col = n % cols;
        const int row = n / cols;
        auto cell = juce::Rectangle<int> (bounds.getX() + col * cellW,
                                          bounds.getY() + row * cellH,
                                          cellW,
                                          cellH).reduced (1);
        const int idx = ids[n];
        const int knobH = juce::jmax (34, cell.getHeight() - 13);
        subKnobs[idx].setBounds (cell.getX(), cell.getY(), cell.getWidth(), knobH);
        subLabels[idx].setBounds (cell.getX(), cell.getBottom() - 14, cell.getWidth(), 13);
    }
}

void IngeniumAudioProcessorEditor::resized()
{
    auto headerArea = getLocalBounds().reduced (22);
    headerArea.setHeight (76);
    bypassButton.setBounds (headerArea.getRight() - 240, headerArea.getY() + 18, 72, 32);
    mainButton.setBounds   (headerArea.getRight() - 158, headerArea.getY() + 18, 64, 32);
    modButton.setBounds    (headerArea.getRight() - 84,  headerArea.getY() + 18, 58, 32);

    auto content = getLocalBounds().reduced (28);
    content.removeFromTop (84);

    if (! modPageVisible)
    {
        auto output = content.removeFromBottom (166);
        content.removeFromBottom (8);
        const int gap = 6;
        const int colW = (content.getWidth() - gap * 5) / 6;

        std::array<juce::Rectangle<int>, 6> p;
        for (int i = 0; i < 6; ++i)
            p[i] = juce::Rectangle<int> (content.getX() + i * (colW + gap), content.getY(), colW, content.getHeight());

        layoutVerticalModule (p[0], 0, { 0, 1, 2 });
        layoutVerticalModule (p[1], 1, { 3, 4, 5 });
        layoutVerticalModule (p[2], 2, { 6, 7, 8, 9, 10, 11 }, &grainCaptureBox, &grainCaptureLabel);
        layoutVerticalModule (p[3], 3, { 12, 13, 14 }, &ghostDivisionBox, &ghostDivisionLabel);
        layoutVerticalModule (p[4], 4, { 15, 16, 17, 18 });
        layoutVerticalModule (p[5], 5, { 19, 20 });

        auto body = output.reduced (12);
        body.removeFromTop (24);
        body.removeFromLeft (350);
        const int section = body.getWidth() / 3;

        auto airArea = body.removeFromLeft (section).reduced (6, 0);
        auto widthArea = body.removeFromLeft (section).reduced (6, 0);
        auto mixArea = body.reduced (6, 0);

        auto layoutOutput = [this] (juce::Rectangle<int> area,
                                    int macroIndex,
                                    std::initializer_list<int> subs)
        {
            auto big = area.removeFromLeft (juce::jmin (112, area.getWidth() / 2));
            mainKnobs[macroIndex].setBounds (big.getX(), big.getY(), big.getWidth(), big.getHeight() - 16);
            mainLabels[macroIndex].setBounds (big.getX(), big.getBottom() - 16, big.getWidth(), 14);
            std::vector<int> ids (subs);
            const int count = juce::jmax (1, static_cast<int> (ids.size()));
            const int w = area.getWidth() / count;
            for (int i = 0; i < static_cast<int> (ids.size()); ++i)
            {
                auto cell = juce::Rectangle<int> (area.getX() + i * w, area.getY(), w, area.getHeight()).reduced (2);
                subKnobs[ids[i]].setBounds (cell.getX(), cell.getY(), cell.getWidth(), cell.getHeight() - 15);
                subLabels[ids[i]].setBounds (cell.getX(), cell.getBottom() - 15, cell.getWidth(), 13);
            }
        };

        layoutOutput (airArea, 6, { 21, 22 });
        layoutOutput (widthArea, 7, { 23 });
        mainKnobs[8].setBounds (mixArea.getCentreX() - 58, mixArea.getY(), 116, mixArea.getHeight() - 16);
        mainLabels[8].setBounds (mixArea.getX(), mixArea.getBottom() - 16, mixArea.getWidth(), 14);
    }
    else
    {
        const int gap = 10;
        const int colW = (content.getWidth() - gap * 2) / 3;
        for (int i = 0; i < 3; ++i)
        {
            auto r = juce::Rectangle<int> (content.getX() + i * (colW + gap), content.getY(), colW, content.getHeight() - 48);
            r.removeFromTop (38);
            lfoLabels[i].setBounds (r.getX(), r.getY() - 29, r.getWidth(), 18);

            auto knobRow = r.removeFromTop (205).reduced (18, 4);
            auto leftArea = knobRow.removeFromLeft (knobRow.getWidth() / 2);
            rateKnobs[i].setBounds (leftArea.reduced (10, 0));
            rateLabels[i].setBounds (leftArea.getX(), leftArea.getBottom() - 20, leftArea.getWidth(), 16);
            depthKnobs[i].setBounds (knobRow.reduced (10, 0));
            depthLabels[i].setBounds (knobRow.getX(), knobRow.getBottom() - 20, knobRow.getWidth(), 16);

            r.removeFromTop (60);
            auto row1 = r.removeFromTop (54).reduced (18, 2);
            shapeLabels[i].setBounds (row1.removeFromTop (13));
            shapeBoxes[i].setBounds (row1);
            auto row2 = r.removeFromTop (54).reduced (18, 2);
            targetLabels[i].setBounds (row2.removeFromTop (13));
            targetBoxes[i].setBounds (row2);
            auto row3 = r.removeFromTop (54).reduced (18, 2);
            divisionLabels[i].setBounds (row3.removeFromTop (13));
            divisionBoxes[i].setBounds (row3.removeFromLeft (juce::jmax (90, row3.getWidth() - 96)));
            row3.removeFromLeft (8);
            syncButtons[i].setBounds (row3);
        }
    }
}