#include "PluginEditor.h"
#include <cmath>

namespace
{
    const juce::Colour shell      { 0xffb7ad99 };
    const juce::Colour shellDark  { 0xff696354 };
    const juce::Colour shellEdge  { 0xff514c42 };
    const juce::Colour panel      { 0xffd6ccb8 };
    const juce::Colour panelDark  { 0xff9b907d };
    const juce::Colour ink        { 0xff191817 };
    const juce::Colour inkDim     { 0xff514c45 };
    const juce::Colour stripe     { 0xff4c5048 };

    const std::array<juce::Colour, 6> effectColours {
        juce::Colour (0xffb9c9a9),
        juce::Colour (0xffd69c87),
        juce::Colour (0xffd8b85f),
        juce::Colour (0xff9fb8c7),
        juce::Colour (0xffb49abf),
        juce::Colour (0xff9fbea9)
    };

    const std::array<juce::Colour, 3> lfoColours {
        juce::Colour (0xff93adbe),
        juce::Colour (0xffa58fb1),
        juce::Colour (0xff91b29c)
    };

    const std::array<juce::Colour, 4> matrixColours {
        juce::Colour (0xffc68f78),
        juce::Colour (0xff8fa9bc),
        juce::Colour (0xffaa92b3),
        juce::Colour (0xff8eaf97)
    };

    void drawScrew (juce::Graphics& g, float x, float y)
    {
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.fillEllipse (x - 5.2f, y - 4.3f, 10.4f, 10.4f);
        g.setColour (juce::Colour (0xff6d695f));
        g.fillEllipse (x - 4.0f, y - 4.0f, 8.0f, 8.0f);
        g.setColour (juce::Colour (0xff2e2c28));
        g.drawLine (x - 2.3f, y - 1.2f, x + 2.3f, y + 1.2f, 1.1f);
    }

    void drawWear (juce::Graphics& g, juce::Rectangle<int> area)
    {
        g.setColour (juce::Colours::black.withAlpha (0.035f));
        for (int i = 0; i < 28; ++i)
        {
            const int x = area.getX() + ((i * 67) % juce::jmax (1, area.getWidth()));
            const int y = area.getY() + ((i * 41) % juce::jmax (1, area.getHeight()));
            const int len = 8 + ((i * 13) % 29);
            g.drawLine (static_cast<float> (x), static_cast<float> (y),
                        static_cast<float> (juce::jmin (area.getRight(), x + len)),
                        static_cast<float> (y + ((i % 3) - 1)),
                        0.7f);
        }
    }

    void drawModule (juce::Graphics& g,
                     juce::Rectangle<int> bounds,
                     juce::Colour colour,
                     const juce::String& title,
                     const juce::String& category)
    {
        auto body = bounds.toFloat();
        g.setColour (panel);
        g.fillRect (body);
        g.setColour (shellEdge.withAlpha (0.78f));
        g.drawRect (body, 1.0f);

        auto header = bounds.removeFromTop (34);
        g.setColour (colour);
        g.fillRect (header);
        g.setColour (colour.darker (0.32f));
        g.drawLine (static_cast<float> (header.getX()), static_cast<float> (header.getBottom()),
                    static_cast<float> (header.getRight()), static_cast<float> (header.getBottom()), 1.0f);

        g.setColour (ink);
        g.setFont (juce::Font (juce::FontOptions (15.0f).withStyle ("Bold")));
        g.drawText (title, header.getX() + 11, header.getY() + 2, header.getWidth() - 92, 22,
                    juce::Justification::centredLeft, false);

        g.setColour (inkDim);
        g.setFont (juce::Font (juce::FontOptions (8.2f).withStyle ("Bold")));
        g.drawText (category, header.getRight() - 78, header.getY() + 4, 66, 18,
                    juce::Justification::centredRight, false);

        g.setColour (inkDim.withAlpha (0.45f));
        g.drawLine (static_cast<float> (bounds.getX() + 14), static_cast<float> (bounds.getBottom() - 22),
                    static_cast<float> (bounds.getRight() - 14), static_cast<float> (bounds.getBottom() - 22), 0.8f);
    }
}

IngeniumAudioProcessorEditor::IngeniumAudioProcessorEditor (IngeniumAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
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
        styleLabel (mainLabels[i], mainParameterNames[i], i < 6 ? 11.2f : 10.2f);
        addAndMakeVisible (mainKnobs[i]);
        addAndMakeVisible (mainLabels[i]);
        mainAttachments[i] = std::make_unique<SliderAttachment> (
            audioProcessor.apvts, mainParameterIDs[i], mainKnobs[i]);
    }

    const juce::StringArray shapes { "SINE", "TRIANGLE", "SAW", "SQUARE", "RANDOM" };
    const juce::StringArray coreModules { "NONE", "AGE", "MELT", "GRAIN", "GHOST", "SMEAR", "CHAOS" };

    for (int i = 0; i < 3; ++i)
    {
        const auto n = juce::String (i + 1);
        styleKnob (rateKnobs[static_cast<size_t> (i)]);
        styleKnob (depthKnobs[static_cast<size_t> (i)]);
        styleLabel (lfoLabels[static_cast<size_t> (i)], "LFO 0" + n, 13.0f);
        styleLabel (rateLabels[static_cast<size_t> (i)], "RATE", 8.8f);
        styleLabel (depthLabels[static_cast<size_t> (i)], "DEPTH", 8.8f);
        styleLabel (shapeLabels[static_cast<size_t> (i)], "SHAPE", 8.2f);
        styleLabel (targetLabels[static_cast<size_t> (i)], "TARGET", 8.2f);

        auto& shape = shapeBoxes[static_cast<size_t> (i)];
        auto& target = targetBoxes[static_cast<size_t> (i)];
        for (int item = 0; item < shapes.size(); ++item) shape.addItem (shapes[item], item + 1);
        for (int item = 0; item < coreModules.size(); ++item) target.addItem (coreModules[item], item + 1);

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
        styleLabel (matrixRouteLabels[static_cast<size_t> (i)], "ROUTE 0" + n, 10.8f);
        styleLabel (matrixSourceLabels[static_cast<size_t> (i)], "SOURCE", 7.7f);
        styleLabel (matrixTargetLabels[static_cast<size_t> (i)], "TARGET", 7.7f);
        styleLabel (matrixModeLabels[static_cast<size_t> (i)], "MODE", 7.7f);
        styleLabel (matrixAmountLabels[static_cast<size_t> (i)], "AMOUNT ±", 7.7f);

        auto& source = matrixSourceBoxes[static_cast<size_t> (i)];
        auto& target = matrixTargetBoxes[static_cast<size_t> (i)];
        auto& mode = matrixModeBoxes[static_cast<size_t> (i)];
        auto& amount = matrixAmountKnobs[static_cast<size_t> (i)];
        for (int item = 0; item < coreModules.size(); ++item)
        {
            source.addItem (coreModules[item], item + 1);
            target.addItem (coreModules[item], item + 1);
        }
        for (int item = 0; item < matrixModes.size(); ++item) mode.addItem (matrixModes[item], item + 1);
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

void IngeniumAudioProcessorEditor::styleLabel (juce::Label& label,
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

    for (auto& knob : mainKnobs) knob.setVisible (! modPageVisible);
    for (auto& label : mainLabels) label.setVisible (! modPageVisible);

    for (int i = 0; i < 3; ++i)
    {
        const bool v = modPageVisible;
        rateKnobs[static_cast<size_t> (i)].setVisible (v);
        depthKnobs[static_cast<size_t> (i)].setVisible (v);
        lfoLabels[static_cast<size_t> (i)].setVisible (v);
        rateLabels[static_cast<size_t> (i)].setVisible (v);
        depthLabels[static_cast<size_t> (i)].setVisible (v);
        shapeLabels[static_cast<size_t> (i)].setVisible (v);
        targetLabels[static_cast<size_t> (i)].setVisible (v);
        shapeBoxes[static_cast<size_t> (i)].setVisible (v);
        targetBoxes[static_cast<size_t> (i)].setVisible (v);
    }

    for (int i = 0; i < 4; ++i)
    {
        const bool v = modPageVisible;
        matrixRouteLabels[static_cast<size_t> (i)].setVisible (v);
        matrixSourceLabels[static_cast<size_t> (i)].setVisible (v);
        matrixTargetLabels[static_cast<size_t> (i)].setVisible (v);
        matrixModeLabels[static_cast<size_t> (i)].setVisible (v);
        matrixAmountLabels[static_cast<size_t> (i)].setVisible (v);
        matrixSourceBoxes[static_cast<size_t> (i)].setVisible (v);
        matrixTargetBoxes[static_cast<size_t> (i)].setVisible (v);
        matrixModeBoxes[static_cast<size_t> (i)].setVisible (v);
        matrixAmountKnobs[static_cast<size_t> (i)].setVisible (v);
    }
}

void IngeniumAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (shellDark);

    auto chassis = getLocalBounds().reduced (8);
    g.setColour (shell);
    g.fillRect (chassis);
    g.setColour (shellEdge);
    g.drawRect (chassis, 2);
    drawWear (g, chassis);

    drawScrew (g, 22.0f, 22.0f);
    drawScrew (g, static_cast<float> (getWidth() - 22), 22.0f);
    drawScrew (g, 22.0f, static_cast<float> (getHeight() - 22));
    drawScrew (g, static_cast<float> (getWidth() - 22), static_cast<float> (getHeight() - 22));

    g.setColour (stripe);
    g.fillRect (28, 22, getWidth() - 56, 58);
    g.setColour (juce::Colour (0xffece4d4));
    g.setFont (juce::Font (juce::FontOptions (30.0f).withStyle ("Bold")));
    g.drawText ("INGENIUM", 45, 28, 270, 30, juce::Justification::centredLeft, false);
    g.setFont (juce::Font (juce::FontOptions (9.2f).withStyle ("Bold")));
    g.drawText ("ANALOG INSTABILITY PROCESSOR", 47, 55, 260, 15,
                juce::Justification::centredLeft, false);

    g.setColour (juce::Colour (0xffd1c6b2));
    g.setFont (juce::Font (juce::FontOptions (8.5f).withStyle ("Bold")));
    g.drawText ("IG-01 / REV.06", getWidth() - 334, 39, 112, 18,
                juce::Justification::centredRight, false);

    auto content = getLocalBounds().reduced (32);
    content.removeFromTop (62);

    if (! modPageVisible)
    {
        auto output = content.removeFromBottom (138);
        content.removeFromBottom (12);

        const int gap = 10;
        const int colW = (content.getWidth() - gap * 2) / 3;
        const int rowH = (content.getHeight() - gap) / 2;

        const std::array<juce::String, 6> titles {
            "AGE", "MELT", "GRAIN", "GHOST", "SMEAR", "CHAOS"
        };
        const std::array<juce::String, 6> cats {
            "WEAR", "WOW / FLUTTER", "MEMORY", "ECHO", "DIFFUSION", "INSTABILITY"
        };

        for (int i = 0; i < 6; ++i)
        {
            const int col = i % 3;
            const int row = i / 3;
            auto r = juce::Rectangle<int> (content.getX() + col * (colW + gap),
                                            content.getY() + row * (rowH + gap),
                                            colW, rowH);
            drawModule (g, r, effectColours[static_cast<size_t> (i)],
                        titles[static_cast<size_t> (i)], cats[static_cast<size_t> (i)]);
        }

        g.setColour (panelDark);
        g.fillRect (output);
        g.setColour (shellEdge);
        g.drawRect (output, 1);
        g.setColour (ink);
        g.setFont (juce::Font (juce::FontOptions (10.0f).withStyle ("Bold")));
        g.drawText ("MASTER / OUTPUT", output.getX() + 14, output.getY() + 8, 150, 16,
                    juce::Justification::centredLeft, false);
        g.setColour (juce::Colour (0xff6f6759));
        g.drawLine (static_cast<float> (output.getX() + 12), static_cast<float> (output.getY() + 29),
                    static_cast<float> (output.getRight() - 12), static_cast<float> (output.getY() + 29), 1.0f);

        const int mixX = output.getX() + output.getWidth() * 2 / 3;
        g.drawLine (static_cast<float> (mixX), static_cast<float> (output.getY() + 38),
                    static_cast<float> (mixX), static_cast<float> (output.getBottom() - 12), 1.0f);
        g.setFont (juce::Font (juce::FontOptions (8.0f).withStyle ("Bold")));
        g.drawText ("AIR / WIDTH", output.getX() + 10, output.getBottom() - 22,
                    output.getWidth() * 2 / 3 - 20, 14, juce::Justification::centred, false);
        g.drawText ("DRY / WET", mixX + 6, output.getBottom() - 22,
                    output.getRight() - mixX - 12, 14, juce::Justification::centred, false);
    }
    else
    {
        auto matrix = content.removeFromBottom (292);
        content.removeFromBottom (10);

        const int gap = 10;
        const int stripW = (content.getWidth() - gap * 2) / 3;
        for (int i = 0; i < 3; ++i)
        {
            auto r = juce::Rectangle<int> (content.getX() + i * (stripW + gap),
                                            content.getY(), stripW, content.getHeight());
            g.setColour (panel);
            g.fillRect (r);
            g.setColour (lfoColours[static_cast<size_t> (i)]);
            g.fillRect (r.removeFromTop (31));
            g.setColour (shellEdge);
            g.drawRect (juce::Rectangle<int> (content.getX() + i * (stripW + gap),
                                               content.getY(), stripW, content.getHeight()), 1);
        }

        g.setColour (panelDark);
        g.fillRect (matrix);
        g.setColour (shellEdge);
        g.drawRect (matrix, 1);
        g.setColour (ink);
        g.setFont (juce::Font (juce::FontOptions (10.0f).withStyle ("Bold")));
        g.drawText ("INTERNAL MOD MATRIX", matrix.getX() + 12, matrix.getY() + 6, 190, 18,
                    juce::Justification::centredLeft, false);

        auto rows = matrix.reduced (10, 32);
        const int routeGap = 6;
        const int routeH = (rows.getHeight() - routeGap * 3) / 4;
        for (int i = 0; i < 4; ++i)
        {
            auto r = juce::Rectangle<int> (rows.getX(), rows.getY() + i * (routeH + routeGap),
                                            rows.getWidth(), routeH);
            g.setColour (matrixColours[static_cast<size_t> (i)].withAlpha (0.88f));
            g.fillRect (r);
            g.setColour (shellEdge.withAlpha (0.8f));
            g.drawRect (r, 1);
        }
    }
}

void IngeniumAudioProcessorEditor::resized()
{
    mainButton.setBounds (getWidth() - 205, 31, 72, 34);
    modButton.setBounds (getWidth() - 123, 31, 72, 34);

    auto content = getLocalBounds().reduced (32);
    content.removeFromTop (62);

    if (! modPageVisible)
    {
        auto output = content.removeFromBottom (138);
        content.removeFromBottom (12);

        const int gap = 10;
        const int colW = (content.getWidth() - gap * 2) / 3;
        const int rowH = (content.getHeight() - gap) / 2;

        for (int i = 0; i < 6; ++i)
        {
            const int col = i % 3;
            const int row = i / 3;
            auto card = juce::Rectangle<int> (content.getX() + col * (colW + gap),
                                               content.getY() + row * (rowH + gap), colW, rowH);
            card.removeFromTop (36);
            card.reduce (12, 8);
            auto label = card.removeFromBottom (20);
            mainLabels[static_cast<size_t> (i)].setBounds (label);
            mainKnobs[static_cast<size_t> (i)].setBounds (card.reduced (18, 0));
        }

        auto out = output.reduced (20, 32);
        const int third = out.getWidth() / 3;
        for (int i = 0; i < 3; ++i)
        {
            auto cell = juce::Rectangle<int> (out.getX() + i * third, out.getY(), third, out.getHeight());
            auto label = cell.removeFromBottom (18);
            mainLabels[static_cast<size_t> (6 + i)].setBounds (label);
            mainKnobs[static_cast<size_t> (6 + i)].setBounds (cell.reduced (24, 0));
        }
    }
    else
    {
        auto matrix = content.removeFromBottom (292);
        content.removeFromBottom (10);

        const int gap = 10;
        const int stripW = (content.getWidth() - gap * 2) / 3;
        for (int i = 0; i < 3; ++i)
        {
            auto strip = juce::Rectangle<int> (content.getX() + i * (stripW + gap),
                                                content.getY(), stripW, content.getHeight()).reduced (12);
            lfoLabels[static_cast<size_t> (i)].setBounds (strip.removeFromTop (26));
            strip.removeFromTop (6);
            auto knobs = strip.removeFromTop (142);
            const int half = knobs.getWidth() / 2;
            auto rate = knobs.removeFromLeft (half).reduced (8, 0);
            auto depth = knobs.reduced (8, 0);
            rateLabels[static_cast<size_t> (i)].setBounds (rate.removeFromBottom (19));
            depthLabels[static_cast<size_t> (i)].setBounds (depth.removeFromBottom (19));
            rateKnobs[static_cast<size_t> (i)].setBounds (rate);
            depthKnobs[static_cast<size_t> (i)].setBounds (depth);
            strip.removeFromTop (3);
            auto shape = strip.removeFromTop (48);
            shapeLabels[static_cast<size_t> (i)].setBounds (shape.removeFromTop (15));
            shapeBoxes[static_cast<size_t> (i)].setBounds (shape.reduced (8, 1));
            auto target = strip.removeFromTop (48);
            targetLabels[static_cast<size_t> (i)].setBounds (target.removeFromTop (15));
            targetBoxes[static_cast<size_t> (i)].setBounds (target.reduced (8, 1));
        }

        auto rows = matrix.reduced (10, 32);
        const int routeGap = 6;
        const int routeH = (rows.getHeight() - routeGap * 3) / 4;
        for (int i = 0; i < 4; ++i)
        {
            auto row = juce::Rectangle<int> (rows.getX(), rows.getY() + i * (routeH + routeGap),
                                              rows.getWidth(), routeH).reduced (8, 3);
            matrixRouteLabels[static_cast<size_t> (i)].setBounds (row.removeFromLeft (84));
            auto source = row.removeFromLeft (190);
            auto target = row.removeFromLeft (190);
            auto mode = row.removeFromLeft (150);
            auto amount = row;

            matrixSourceLabels[static_cast<size_t> (i)].setBounds (source.removeFromTop (14));
            matrixTargetLabels[static_cast<size_t> (i)].setBounds (target.removeFromTop (14));
            matrixModeLabels[static_cast<size_t> (i)].setBounds (mode.removeFromTop (14));
            matrixAmountLabels[static_cast<size_t> (i)].setBounds (amount.removeFromTop (14));
            matrixSourceBoxes[static_cast<size_t> (i)].setBounds (source.reduced (8, 1));
            matrixTargetBoxes[static_cast<size_t> (i)].setBounds (target.reduced (8, 1));
            matrixModeBoxes[static_cast<size_t> (i)].setBounds (mode.reduced (8, 1));
            matrixAmountKnobs[static_cast<size_t> (i)].setBounds (amount.reduced (18, -3));
        }
    }
}
