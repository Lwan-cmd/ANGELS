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

    setSize (1020, 700);
    setResizable (true, true);
    setResizeLimits (880, 610, 1340, 900);

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

    const juce::StringArray targets {
        "NONE", "AGE", "MELT", "GRAIN", "GHOST", "SMEAR", "CHAOS"
    };

    for (int i = 0; i < 3; ++i)
    {
        const auto n = juce::String (i + 1);

        styleKnob (rateKnobs[static_cast<size_t> (i)]);
        styleKnob (depthKnobs[static_cast<size_t> (i)]);

        styleLabel (
            lfoLabels[static_cast<size_t> (i)],
            "LFO 0" + n,
            16.0f);

        styleLabel (
            rateLabels[static_cast<size_t> (i)],
            "RATE",
            10.5f);

        styleLabel (
            depthLabels[static_cast<size_t> (i)],
            "DEPTH",
            10.5f);

        styleLabel (
            shapeLabels[static_cast<size_t> (i)],
            "SHAPE",
            9.5f);

        styleLabel (
            targetLabels[static_cast<size_t> (i)],
            "TARGET",
            9.5f);

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

        rateAttachments[static_cast<size_t> (i)] =
            std::make_unique<SliderAttachment> (
                audioProcessor.apvts,
                "lfo" + n + "Rate",
                rateKnobs[static_cast<size_t> (i)]);

        depthAttachments[static_cast<size_t> (i)] =
            std::make_unique<SliderAttachment> (
                audioProcessor.apvts,
                "lfo" + n + "Depth",
                depthKnobs[static_cast<size_t> (i)]);

        shapeAttachments[static_cast<size_t> (i)] =
            std::make_unique<ComboAttachment> (
                audioProcessor.apvts,
                "lfo" + n + "Shape",
                shape);

        targetAttachments[static_cast<size_t> (i)] =
            std::make_unique<ComboAttachment> (
                audioProcessor.apvts,
                "lfo" + n + "Dest",
                target);
    }

    updatePageVisibility();
}

IngeniumAudioProcessorEditor::~IngeniumAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void IngeniumAudioProcessorEditor::styleKnob (juce::Slider& slider)
{
    slider.setSliderStyle (
        juce::Slider::RotaryHorizontalVerticalDrag);

    slider.setTextBoxStyle (
        juce::Slider::NoTextBox,
        false,
        0,
        0);

    slider.setMouseDragSensitivity (180);
}

void IngeniumAudioProcessorEditor::styleLabel (
    juce::Label& label,
    const juce::String& text,
    float size)
{
    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);

    label.setFont (
        juce::Font (
            juce::FontOptions (size).withStyle ("Bold")));

    label.setColour (
        juce::Label::textColourId,
        ink);
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
    mainButton.setToggleState (
        ! modPageVisible,
        juce::dontSendNotification);

    modButton.setToggleState (
        modPageVisible,
        juce::dontSendNotification);

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
}

void IngeniumAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (shell);

    auto whole = getLocalBounds().toFloat();

    g.setColour (shellEdge);
    g.drawRoundedRectangle (
        whole.reduced (1.0f),
        15.0f,
        1.2f);

    drawScrew (g, 18.0f, 18.0f);
    drawScrew (g, static_cast<float> (getWidth() - 18), 18.0f);
    drawScrew (g, 18.0f, static_cast<float> (getHeight() - 18));
    drawScrew (
        g,
        static_cast<float> (getWidth() - 18),
        static_cast<float> (getHeight() - 18));

    g.setColour (ink);
    g.setFont (
        juce::Font (
            juce::FontOptions (32.0f).withStyle ("Bold")));

    g.drawText (
        "INGENIUM",
        46,
        21,
        310,
        38,
        juce::Justification::centredLeft,
        false);

    g.setColour (inkDim);
    g.setFont (
        juce::Font (
            juce::FontOptions (10.0f).withStyle ("Bold")));

    g.drawText (
        "ANALOG INSTABILITY PROCESSOR",
        48,
        57,
        310,
        18,
        juce::Justification::centredLeft,
        false);

    g.drawText (
        "IG-01  /  v0.4",
        getWidth() - 330,
        24,
        105,
        18,
        juce::Justification::centredRight,
        false);

    auto content = getLocalBounds().reduced (34);
    content.removeFromTop (72);

    if (! modPageVisible)
    {
        auto output = content.removeFromBottom (142);
        content.removeFromBottom (12);

        const int gap = 12;
        const int columnWidth =
            (content.getWidth() - gap * 2) / 3;

        const int rowHeight =
            (content.getHeight() - gap) / 2;

        const std::array<juce::String, 6> titles {
            "AGE", "MELT", "GRAIN",
            "GHOST", "SMEAR", "CHAOS"
        };

        const std::array<juce::String, 6> categories {
            "TONE", "TEXTURE", "TEXTURE",
            "SPACE", "MOTION", "MODULATION"
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

            drawModule (
                g,
                r,
                effectColours[static_cast<size_t> (i)],
                titles[static_cast<size_t> (i)],
                categories[static_cast<size_t> (i)]);
        }

        g.setColour (juce::Colour (0xffebe6dc));
        g.fillRoundedRectangle (
            output.toFloat(),
            13.0f);

        g.setColour (shellEdge.withAlpha (0.72f));
        g.drawRoundedRectangle (
            output.toFloat().reduced (0.5f),
            13.0f,
            1.0f);

        g.setColour (inkDim);
        g.setFont (
            juce::Font (
                juce::FontOptions (10.0f).withStyle ("Bold")));

        g.drawText (
            "OUTPUT / FINISH",
            output.getX() + 18,
            output.getY() + 12,
            150,
            18,
            juce::Justification::centredLeft,
            false);

        const int mixX =
            output.getX() + output.getWidth() * 2 / 3;

        g.setColour (shellEdge.withAlpha (0.85f));
        g.drawLine (
            static_cast<float> (mixX),
            static_cast<float> (output.getY() + 18),
            static_cast<float> (mixX),
            static_cast<float> (output.getBottom() - 18),
            1.0f);

        g.setColour (inkDim.withAlpha (0.72f));
        g.setFont (
            juce::Font (
                juce::FontOptions (8.5f)));

        g.drawText (
            "AIR  /  WIDTH",
            output.getX() + 18,
            output.getBottom() - 26,
            output.getWidth() * 2 / 3 - 38,
            14,
            juce::Justification::centred,
            false);

        g.drawText (
            "MIX  —  OUTPUT ONLY",
            mixX + 8,
            output.getBottom() - 26,
            output.getRight() - mixX - 26,
            14,
            juce::Justification::centred,
            false);
    }
    else
    {
        const int gap = 16;
        const int stripWidth =
            (content.getWidth() - gap * 2) / 3;

        for (int i = 0; i < 3; ++i)
        {
            const auto strip = juce::Rectangle<int> (
                content.getX() + i * (stripWidth + gap),
                content.getY(),
                stripWidth,
                content.getHeight());

            g.setColour (
                lfoColours[static_cast<size_t> (i)]);

            g.fillRoundedRectangle (
                strip.toFloat(),
                13.0f);

            g.setColour (
                lfoColours[static_cast<size_t> (i)]
                    .darker (0.18f)
                    .withAlpha (0.55f));

            g.drawRoundedRectangle (
                strip.toFloat().reduced (0.5f),
                13.0f,
                1.0f);

            g.setColour (inkDim.withAlpha (0.60f));

            auto wave = strip.reduced (30);
            wave.removeFromTop (
                juce::jmax (0, wave.getHeight() - 70));

            juce::Path p;
            const float midY =
                static_cast<float> (wave.getCentreY());

            p.startNewSubPath (
                static_cast<float> (wave.getX()),
                midY);

            const int points = 64;

            for (int k = 1; k < points; ++k)
            {
                const float norm =
                    static_cast<float> (k)
                    / static_cast<float> (points - 1);

                float y = 0.0f;

                if (i == 0)
                {
                    y = std::sin (
                        norm * juce::MathConstants<float>::twoPi * 2.0f);
                }
                else if (i == 1)
                {
                    const float phase =
                        std::fmod (norm * 2.0f, 1.0f);

                    y = 1.0f
                        - 4.0f
                        * std::abs (
                            std::round (phase - 0.25f)
                            - (phase - 0.25f));
                }
                else
                {
                    const int step =
                        static_cast<int> (norm * 9.0f);

                    y = static_cast<float> (
                        ((step * 37) % 11) - 5) / 5.0f;
                }

                const float px =
                    static_cast<float> (wave.getX())
                    + norm * static_cast<float> (wave.getWidth());

                const float py =
                    midY - y * 22.0f;

                p.lineTo (px, py);
            }

            g.strokePath (
                p,
                juce::PathStrokeType (1.6f));
        }

        g.setColour (inkDim);
        g.setFont (
            juce::Font (
                juce::FontOptions (9.0f)));

        g.drawText (
            "LFO TARGETS: AGE / MELT / GRAIN / GHOST / SMEAR / CHAOS",
            content.getX(),
            content.getBottom() - 16,
            content.getWidth(),
            14,
            juce::Justification::centred,
            false);
    }
}

void IngeniumAudioProcessorEditor::resized()
{
    mainButton.setBounds (
        getWidth() - 202,
        24,
        72,
        38);

    modButton.setBounds (
        getWidth() - 120,
        24,
        72,
        38);

    auto content = getLocalBounds().reduced (34);
    content.removeFromTop (72);

    if (! modPageVisible)
    {
        auto output = content.removeFromBottom (142);
        content.removeFromBottom (12);

        const int gap = 12;
        const int columnWidth =
            (content.getWidth() - gap * 2) / 3;

        const int rowHeight =
            (content.getHeight() - gap) / 2;

        for (int i = 0; i < 6; ++i)
        {
            const int col = i % 3;
            const int row = i / 3;

            auto card = juce::Rectangle<int> (
                content.getX() + col * (columnWidth + gap),
                content.getY() + row * (rowHeight + gap),
                columnWidth,
                rowHeight);

            card.reduce (12, 42);

            mainLabels[static_cast<size_t> (i)].setBounds (
                card.removeFromBottom (22));

            mainKnobs[static_cast<size_t> (i)].setBounds (
                card.reduced (8, 3));
        }

        auto outputInner = output.reduced (26, 24);
        outputInner.removeFromTop (12);

        const int third = outputInner.getWidth() / 3;

        for (int i = 0; i < 3; ++i)
        {
            auto cell = juce::Rectangle<int> (
                outputInner.getX() + i * third,
                outputInner.getY(),
                third,
                outputInner.getHeight());

            auto labelArea = cell.removeFromBottom (20);

            mainLabels[static_cast<size_t> (6 + i)]
                .setBounds (labelArea);

            mainKnobs[static_cast<size_t> (6 + i)]
                .setBounds (cell.reduced (24, 2));
        }
    }
    else
    {
        const int gap = 16;
        const int stripWidth =
            (content.getWidth() - gap * 2) / 3;

        for (int i = 0; i < 3; ++i)
        {
            auto strip = juce::Rectangle<int> (
                content.getX() + i * (stripWidth + gap),
                content.getY(),
                stripWidth,
                content.getHeight())
                .reduced (18);

            lfoLabels[static_cast<size_t> (i)]
                .setBounds (strip.removeFromTop (34));

            strip.removeFromTop (10);

            auto knobsArea = strip.removeFromTop (245);
            const int half = knobsArea.getWidth() / 2;

            auto rateArea =
                knobsArea.removeFromLeft (half).reduced (4, 0);

            auto depthArea =
                knobsArea.reduced (4, 0);

            rateLabels[static_cast<size_t> (i)]
                .setBounds (rateArea.removeFromBottom (24));

            depthLabels[static_cast<size_t> (i)]
                .setBounds (depthArea.removeFromBottom (24));

            rateKnobs[static_cast<size_t> (i)]
                .setBounds (rateArea);

            depthKnobs[static_cast<size_t> (i)]
                .setBounds (depthArea);

            strip.removeFromTop (6);

            auto shapeRow = strip.removeFromTop (64);

            shapeLabels[static_cast<size_t> (i)]
                .setBounds (shapeRow.removeFromTop (18));

            shapeBoxes[static_cast<size_t> (i)]
                .setBounds (shapeRow.reduced (10, 2));

            strip.removeFromTop (4);

            auto targetRow = strip.removeFromTop (64);

            targetLabels[static_cast<size_t> (i)]
                .setBounds (targetRow.removeFromTop (18));

            targetBoxes[static_cast<size_t> (i)]
                .setBounds (targetRow.reduced (10, 2));
        }
    }
}
