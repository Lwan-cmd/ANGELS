#include "PluginEditor.h"

namespace
{
    const juce::Colour shell      { 0xfff3efe6 };
    const juce::Colour shellEdge  { 0xffbdb5a8 };
    const juce::Colour ink        { 0xff202020 };
    const juce::Colour inkDim     { 0xff62605b };

    const std::array<juce::Colour, 6> effectColours {
        juce::Colour (0xffdce9d3),
        juce::Colour (0xffffc8b7),
        juce::Colour (0xffffe0a2),
        juce::Colour (0xffcfe4f3),
        juce::Colour (0xffddccec),
        juce::Colour (0xffcfe8df)
    };

    const std::array<juce::Colour, 3> lfoColours {
        juce::Colour (0xffcfe4f3),
        juce::Colour (0xffddccec),
        juce::Colour (0xffcfe8df)
    };

    const std::array<juce::Colour, 4> matrixColours {
        juce::Colour (0xffffd8c9),
        juce::Colour (0xffd9e8f6),
        juce::Colour (0xffe4d6f1),
        juce::Colour (0xffd9ecdf)
    };

    void drawScrew (juce::Graphics& g, float x, float y)
    {
        g.setColour (juce::Colours::black.withAlpha (0.14f));
        g.fillEllipse (x - 5.0f, y - 4.0f, 10.0f, 10.0f);

        g.setColour (juce::Colour (0xff383838));
        g.fillEllipse (x - 4.0f, y - 4.0f, 8.0f, 8.0f);

        g.setColour (juce::Colour (0xff9e9a92));
        g.drawLine (x - 2.0f, y, x + 2.0f, y, 1.0f);
    }

    void drawModule (juce::Graphics& g,
                     juce::Rectangle<int> bounds,
                     juce::Colour colour,
                     const juce::String& title,
                     const juce::String& category)
    {
        auto r = bounds.toFloat();

        g.setColour (colour);
        g.fillRoundedRectangle (r, 13.0f);

        g.setColour (colour.darker (0.16f).withAlpha (0.55f));
        g.drawRoundedRectangle (r.reduced (0.5f), 13.0f, 1.0f);

        g.setColour (colour.darker (0.36f));
        g.fillEllipse (r.getX() + 15.0f, r.getY() + 17.0f, 9.0f, 9.0f);

        g.setColour (ink);
        g.setFont (juce::Font (
            juce::FontOptions (17.0f).withStyle ("Bold")));

        g.drawText (
            title,
            bounds.getX() + 32,
            bounds.getY() + 10,
            bounds.getWidth() - 84,
            25,
            juce::Justification::centredLeft,
            false);

        g.setColour (inkDim);
        g.setFont (juce::Font (
            juce::FontOptions (9.3f).withStyle ("Bold")));

        g.drawText (
            category,
            bounds.getRight() - 92,
            bounds.getY() + 12,
            74,
            20,
            juce::Justification::centredRight,
            false);
    }
}

IngeniumAudioProcessorEditor::IngeniumAudioProcessorEditor (
    IngeniumAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p)
{
    setLookAndFeel (&ingeniumLook);

    setSize (1120, 780);
    setResizable (true, true);
    setResizeLimits (980, 680, 1480, 1000);

    mainButton.setClickingTogglesState (false);
    modButton.setClickingTogglesState (false);

    mainButton.onClick = [this] { showMainPage(); };
    modButton.onClick = [this] { showModPage(); };

    addAndMakeVisible (mainButton);
    addAndMakeVisible (modButton);

    for (size_t i = 0; i < mainKnobs.size(); ++i)
    {
        styleKnob (mainKnobs[i]);
        styleLabel (
            mainLabels[i],
            mainParameterNames[i],
            i < 6 ? 12.0f : 11.0f);

        addAndMakeVisible (mainKnobs[i]);
        addAndMakeVisible (mainLabels[i]);

        mainAttachments[i] = std::make_unique<SliderAttachment> (
            audioProcessor.apvts,
            mainParameterIDs[i],
            mainKnobs[i]);
    }

    const juce::StringArray shapes {
        "SINE", "TRIANGLE", "SAW", "SQUARE", "RANDOM"
    };

    const juce::StringArray coreModules {
        "NONE", "AGE", "MELT", "GRAIN", "GHOST", "SMEAR", "CHAOS"
    };

    for (int i = 0; i < 3; ++i)
    {
        const auto n = juce::String (i + 1);

        styleKnob (rateKnobs[static_cast<size_t> (i)]);
        styleKnob (depthKnobs[static_cast<size_t> (i)]);

        styleLabel (lfoLabels[static_cast<size_t> (i)], "LFO 0" + n, 15.0f);
        styleLabel (rateLabels[static_cast<size_t> (i)], "RATE", 9.5f);
        styleLabel (depthLabels[static_cast<size_t> (i)], "DEPTH", 9.5f);
        styleLabel (shapeLabels[static_cast<size_t> (i)], "SHAPE", 8.8f);
        styleLabel (targetLabels[static_cast<size_t> (i)], "TARGET", 8.8f);

        auto& shape = shapeBoxes[static_cast<size_t> (i)];
        auto& target = targetBoxes[static_cast<size_t> (i)];

        for (int item = 0; item < shapes.size(); ++item)
            shape.addItem (shapes[item], item + 1);

        for (int item = 0; item < coreModules.size(); ++item)
            target.addItem (coreModules[item], item + 1);

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

    const juce::StringArray matrixModes { "CONTROL", "AUDIO" };

    for (int i = 0; i < 4; ++i)
    {
        const auto n = juce::String (i + 1);

        styleLabel (matrixRouteLabels[static_cast<size_t> (i)], "ROUTE 0" + n, 12.0f);
        styleLabel (matrixSourceLabels[static_cast<size_t> (i)], "SOURCE", 8.5f);
        styleLabel (matrixTargetLabels[static_cast<size_t> (i)], "TARGET", 8.5f);
        styleLabel (matrixModeLabels[static_cast<size_t> (i)], "MODE", 8.5f);
        styleLabel (matrixAmountLabels[static_cast<size_t> (i)], "AMOUNT ±", 8.5f);

        auto& source = matrixSourceBoxes[static_cast<size_t> (i)];
        auto& target = matrixTargetBoxes[static_cast<size_t> (i)];
        auto& mode = matrixModeBoxes[static_cast<size_t> (i)];
        auto& amount = matrixAmountKnobs[static_cast<size_t> (i)];

        for (int item = 0; item < coreModules.size(); ++item)
        {
            source.addItem (coreModules[item], item + 1);
            target.addItem (coreModules[item], item + 1);
        }

        for (int item = 0; item < matrixModes.size(); ++item)
            mode.addItem (matrixModes[item], item + 1);

        styleKnob (amount);
        amount.setMouseDragSensitivity (220);

        addAndMakeVisible (matrixRouteLabels[static_cast<size_t> (i)]);
        addAndMakeVisible (matrixSourceLabels[static_cast<size_t> (i)]);
        addAndMakeVisible (matrixTargetLabels[static_cast<size_t> (i)]);
        addAndMakeVisible (matrixModeLabels[static_cast<size_t> (i)]);
        addAndMakeVisible (matrixAmountLabels[static_cast<size_t> (i)]);
        addAndMakeVisible (source);
        addAndMakeVisible (target);
        addAndMakeVisible (mode);
        addAndMakeVisible (amount);

        matrixSourceAttachments[static_cast<size_t> (i)] = std::make_unique<ComboAttachment> (
            audioProcessor.apvts, "matrix" + n + "Source", source);

        matrixTargetAttachments[static_cast<size_t> (i)] = std::make_unique<ComboAttachment> (
            audioProcessor.apvts, "matrix" + n + "Dest", target);

        matrixModeAttachments[static_cast<size_t> (i)] = std::make_unique<ComboAttachment> (
            audioProcessor.apvts, "matrix" + n + "Mode", mode);

        matrixAmountAttachments[static_cast<size_t> (i)] = std::make_unique<SliderAttachment> (
            audioProcessor.apvts, "matrix" + n + "Amount", amount);
    }

    updatePageVisibility();
}

IngeniumAudioProcessorEditor::~IngeniumAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void IngeniumAudioProcessorEditor::styleKnob (juce::Slider& slider)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setMouseDragSensitivity (180);
}

void IngeniumAudioProcessorEditor::styleLabel (
    juce::Label& label,
    const juce::String& text,
    float size)
{
    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::Font (juce::FontOptions (size).withStyle ("Bold")));
    label.setColour (juce::Label::textColourId, ink);
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

    for (auto& knob : mainKnobs)
        knob.setVisible (! modPageVisible);

    for (auto& label : mainLabels)
        label.setVisible (! modPageVisible);

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

    for (int i = 0; i < 4; ++i)
    {
        const bool visible = modPageVisible;
        matrixRouteLabels[static_cast<size_t> (i)].setVisible (visible);
        matrixSourceLabels[static_cast<size_t> (i)].setVisible (visible);
        matrixTargetLabels[static_cast<size_t> (i)].setVisible (visible);
        matrixModeLabels[static_cast<size_t> (i)].setVisible (visible);
        matrixAmountLabels[static_cast<size_t> (i)].setVisible (visible);
        matrixSourceBoxes[static_cast<size_t> (i)].setVisible (visible);
        matrixTargetBoxes[static_cast<size_t> (i)].setVisible (visible);
        matrixModeBoxes[static_cast<size_t> (i)].setVisible (visible);
        matrixAmountKnobs[static_cast<size_t> (i)].setVisible (visible);
    }
}

void IngeniumAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (shell);

    auto whole = getLocalBounds().toFloat();
    g.setColour (shellEdge);
    g.drawRoundedRectangle (whole.reduced (1.0f), 15.0f, 1.2f);

    drawScrew (g, 18.0f, 18.0f);
    drawScrew (g, static_cast<float> (getWidth() - 18), 18.0f);
    drawScrew (g, 18.0f, static_cast<float> (getHeight() - 18));
    drawScrew (g, static_cast<float> (getWidth() - 18), static_cast<float> (getHeight() - 18));

    g.setColour (ink);
    g.setFont (juce::Font (juce::FontOptions (32.0f).withStyle ("Bold")));
    g.drawText ("INGENIUM", 46, 21, 310, 38, juce::Justification::centredLeft, false);

    g.setColour (inkDim);
    g.setFont (juce::Font (juce::FontOptions (10.0f).withStyle ("Bold")));
    g.drawText ("ANALOG INSTABILITY PROCESSOR", 48, 57, 310, 18,
                juce::Justification::centredLeft, false);
    g.drawText ("IG-01  /  v0.5", getWidth() - 340, 24, 115, 18,
                juce::Justification::centredRight, false);

    auto content = getLocalBounds().reduced (34);
    content.removeFromTop (72);

    if (! modPageVisible)
    {
        auto output = content.removeFromBottom (142);
        content.removeFromBottom (12);

        const int gap = 12;
        const int columnWidth = (content.getWidth() - gap * 2) / 3;
        const int rowHeight = (content.getHeight() - gap) / 2;

        const std::array<juce::String, 6> titles {
            "AGE", "MELT", "GRAIN", "GHOST", "SMEAR", "CHAOS"
        };
        const std::array<juce::String, 6> categories {
            "TONE", "TEXTURE", "TEXTURE", "SPACE", "MOTION", "MODULATION"
        };

        for (int i = 0; i < 6; ++i)
        {
            const int col = i % 3;
            const int row = i / 3;
            const auto r = juce::Rectangle<int> (
                content.getX() + col * (columnWidth + gap),
                content.getY() + row * (rowHeight + gap),
                columnWidth,
                rowHeight);
            drawModule (g, r, effectColours[static_cast<size_t> (i)],
                        titles[static_cast<size_t> (i)], categories[static_cast<size_t> (i)]);
        }

        g.setColour (juce::Colour (0xffebe6dc));
        g.fillRoundedRectangle (output.toFloat(), 13.0f);
        g.setColour (shellEdge.withAlpha (0.72f));
        g.drawRoundedRectangle (output.toFloat().reduced (0.5f), 13.0f, 1.0f);

        g.setColour (inkDim);
        g.setFont (juce::Font (juce::FontOptions (10.0f).withStyle ("Bold")));
        g.drawText ("OUTPUT / FINISH", output.getX() + 18, output.getY() + 12,
                    150, 18, juce::Justification::centredLeft, false);

        const int mixX = output.getX() + output.getWidth() * 2 / 3;
        g.setColour (shellEdge.withAlpha (0.85f));
        g.drawLine (static_cast<float> (mixX), static_cast<float> (output.getY() + 18),
                    static_cast<float> (mixX), static_cast<float> (output.getBottom() - 18), 1.0f);

        g.setColour (inkDim.withAlpha (0.72f));
        g.setFont (juce::Font (juce::FontOptions (8.5f)));
        g.drawText ("AIR  /  WIDTH", output.getX() + 18, output.getBottom() - 26,
                    output.getWidth() * 2 / 3 - 38, 14, juce::Justification::centred, false);
        g.drawText ("MIX  —  OUTPUT ONLY", mixX + 8, output.getBottom() - 26,
                    output.getRight() - mixX - 26, 14, juce::Justification::centred, false);
    }
    else
    {
        auto matrixArea = content.removeFromBottom (318);
        content.removeFromBottom (12);

        const int gap = 14;
        const int stripWidth = (content.getWidth() - gap * 2) / 3;

        for (int i = 0; i < 3; ++i)
        {
            const auto strip = juce::Rectangle<int> (
                content.getX() + i * (stripWidth + gap), content.getY(),
                stripWidth, content.getHeight());
            g.setColour (lfoColours[static_cast<size_t> (i)]);
            g.fillRoundedRectangle (strip.toFloat(), 13.0f);
            g.setColour (lfoColours[static_cast<size_t> (i)].darker (0.18f).withAlpha (0.55f));
            g.drawRoundedRectangle (strip.toFloat().reduced (0.5f), 13.0f, 1.0f);
        }

        g.setColour (juce::Colour (0xffece8df));
        g.fillRoundedRectangle (matrixArea.toFloat(), 13.0f);
        g.setColour (shellEdge.withAlpha (0.80f));
        g.drawRoundedRectangle (matrixArea.toFloat().reduced (0.5f), 13.0f, 1.0f);

        g.setColour (ink);
        g.setFont (juce::Font (juce::FontOptions (15.0f).withStyle ("Bold")));
        g.drawText ("INTERNAL MOD MATRIX", matrixArea.getX() + 18, matrixArea.getY() + 12,
                    230, 22, juce::Justification::centredLeft, false);

        g.setColour (inkDim);
        g.setFont (juce::Font (juce::FontOptions (8.8f).withStyle ("Bold")));
        g.drawText ("CONTROL = SMOOTHED CROSS-MOD   /   AUDIO = ONE-SAMPLE CROSS-FM",
                    matrixArea.getX() + 235, matrixArea.getY() + 14,
                    matrixArea.getWidth() - 255, 18, juce::Justification::centredRight, false);

        auto lanes = matrixArea.reduced (16, 44);
        const int laneGap = 8;
        const int laneHeight = (lanes.getHeight() - laneGap * 3) / 4;

        for (int i = 0; i < 4; ++i)
        {
            const auto lane = juce::Rectangle<int> (
                lanes.getX(), lanes.getY() + i * (laneHeight + laneGap),
                lanes.getWidth(), laneHeight);
            g.setColour (matrixColours[static_cast<size_t> (i)]);
            g.fillRoundedRectangle (lane.toFloat(), 10.0f);
            g.setColour (matrixColours[static_cast<size_t> (i)].darker (0.18f).withAlpha (0.55f));
            g.drawRoundedRectangle (lane.toFloat().reduced (0.5f), 10.0f, 1.0f);

            const float lineY = static_cast<float> (lane.getCentreY() + 7);
            const float fromX = static_cast<float> (lane.getX() + lane.getWidth() * 0.30f);
            const float toX = static_cast<float> (lane.getX() + lane.getWidth() * 0.58f);
            g.setColour (inkDim.withAlpha (0.48f));
            g.drawLine (fromX, lineY, toX, lineY, 1.4f);
            juce::Path arrow;
            arrow.addTriangle (toX - 6.0f, lineY - 4.0f, toX - 6.0f, lineY + 4.0f, toX, lineY);
            g.fillPath (arrow);
        }

        g.setColour (inkDim.withAlpha (0.72f));
        g.setFont (juce::Font (juce::FontOptions (8.5f)));
        g.drawText ("SOURCES / TARGETS: AGE · MELT · GRAIN · GHOST · SMEAR · CHAOS    /    MIX STAYS OUTSIDE THE MATRIX",
                    matrixArea.getX() + 20, matrixArea.getBottom() - 22,
                    matrixArea.getWidth() - 40, 14, juce::Justification::centred, false);
    }
}

void IngeniumAudioProcessorEditor::resized()
{
    mainButton.setBounds (getWidth() - 202, 24, 72, 38);
    modButton.setBounds (getWidth() - 120, 24, 72, 38);

    auto content = getLocalBounds().reduced (34);
    content.removeFromTop (72);

    if (! modPageVisible)
    {
        auto output = content.removeFromBottom (142);
        content.removeFromBottom (12);

        const int gap = 12;
        const int columnWidth = (content.getWidth() - gap * 2) / 3;
        const int rowHeight = (content.getHeight() - gap) / 2;

        for (int i = 0; i < 6; ++i)
        {
            const int col = i % 3;
            const int row = i / 3;
            auto card = juce::Rectangle<int> (
                content.getX() + col * (columnWidth + gap),
                content.getY() + row * (rowHeight + gap), columnWidth, rowHeight);
            card.reduce (12, 42);
            mainLabels[static_cast<size_t> (i)].setBounds (card.removeFromBottom (22));
            mainKnobs[static_cast<size_t> (i)].setBounds (card.reduced (8, 3));
        }

        auto outputInner = output.reduced (26, 24);
        outputInner.removeFromTop (12);
        const int third = outputInner.getWidth() / 3;

        for (int i = 0; i < 3; ++i)
        {
            auto cell = juce::Rectangle<int> (
                outputInner.getX() + i * third, outputInner.getY(), third, outputInner.getHeight());
            auto labelArea = cell.removeFromBottom (20);
            mainLabels[static_cast<size_t> (6 + i)].setBounds (labelArea);
            mainKnobs[static_cast<size_t> (6 + i)].setBounds (cell.reduced (24, 2));
        }
    }
    else
    {
        auto matrixArea = content.removeFromBottom (318);
        content.removeFromBottom (12);

        const int gap = 14;
        const int stripWidth = (content.getWidth() - gap * 2) / 3;

        for (int i = 0; i < 3; ++i)
        {
            auto strip = juce::Rectangle<int> (
                content.getX() + i * (stripWidth + gap), content.getY(),
                stripWidth, content.getHeight()).reduced (16);

            lfoLabels[static_cast<size_t> (i)].setBounds (strip.removeFromTop (28));
            strip.removeFromTop (4);

            auto knobsArea = strip.removeFromTop (145);
            const int half = knobsArea.getWidth() / 2;
            auto rateArea = knobsArea.removeFromLeft (half).reduced (5, 0);
            auto depthArea = knobsArea.reduced (5, 0);
            rateLabels[static_cast<size_t> (i)].setBounds (rateArea.removeFromBottom (20));
            depthLabels[static_cast<size_t> (i)].setBounds (depthArea.removeFromBottom (20));
            rateKnobs[static_cast<size_t> (i)].setBounds (rateArea);
            depthKnobs[static_cast<size_t> (i)].setBounds (depthArea);

            auto selectors = strip.removeFromTop (58);
            auto leftCell = selectors.removeFromLeft (selectors.getWidth() / 2);
            auto rightCell = selectors;
            shapeLabels[static_cast<size_t> (i)].setBounds (leftCell.removeFromTop (16));
            targetLabels[static_cast<size_t> (i)].setBounds (rightCell.removeFromTop (16));
            shapeBoxes[static_cast<size_t> (i)].setBounds (leftCell.reduced (4, 0));
            targetBoxes[static_cast<size_t> (i)].setBounds (rightCell.reduced (4, 0));
        }

        auto lanes = matrixArea.reduced (16, 44);
        const int laneGap = 8;
        const int laneHeight = (lanes.getHeight() - laneGap * 3) / 4;

        for (int i = 0; i < 4; ++i)
        {
            auto lane = juce::Rectangle<int> (
                lanes.getX(), lanes.getY() + i * (laneHeight + laneGap),
                lanes.getWidth(), laneHeight).reduced (10, 6);

            const int routeWidth = 88;
            const int amountWidth = 88;
            const int modeWidth = 122;
            const int boxGap = 10;

            auto routeArea = lane.removeFromLeft (routeWidth);
            matrixRouteLabels[static_cast<size_t> (i)].setBounds (routeArea);
            lane.removeFromLeft (boxGap);

            const int comboWidth = (lane.getWidth() - amountWidth - modeWidth - boxGap * 4) / 2;
            auto sourceArea = lane.removeFromLeft (comboWidth);
            lane.removeFromLeft (boxGap);
            auto targetArea = lane.removeFromLeft (comboWidth);
            lane.removeFromLeft (boxGap);
            auto modeArea = lane.removeFromLeft (modeWidth);
            lane.removeFromLeft (boxGap);
            auto amountArea = lane.removeFromLeft (amountWidth);

            matrixSourceLabels[static_cast<size_t> (i)].setBounds (sourceArea.removeFromTop (14));
            matrixTargetLabels[static_cast<size_t> (i)].setBounds (targetArea.removeFromTop (14));
            matrixModeLabels[static_cast<size_t> (i)].setBounds (modeArea.removeFromTop (14));
            matrixAmountLabels[static_cast<size_t> (i)].setBounds (amountArea.removeFromTop (14));
            matrixSourceBoxes[static_cast<size_t> (i)].setBounds (sourceArea.reduced (2, 1));
            matrixTargetBoxes[static_cast<size_t> (i)].setBounds (targetArea.reduced (2, 1));
            matrixModeBoxes[static_cast<size_t> (i)].setBounds (modeArea.reduced (2, 1));
            matrixAmountKnobs[static_cast<size_t> (i)].setBounds (amountArea.expanded (7, 15));
        }
    }
}
