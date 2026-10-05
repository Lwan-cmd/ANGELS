#pragma once

#include <JuceHeader.h>
#include <cmath>

class IngeniumLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    IngeniumLookAndFeel()
    {
        const auto cream = juce::Colour (0xffddd3bf);
        const auto dark  = juce::Colour (0xff1e1d1a);
        const auto edge  = juce::Colour (0xff8e8371);

        setColour (juce::ComboBox::backgroundColourId, cream);
        setColour (juce::ComboBox::textColourId, dark);
        setColour (juce::ComboBox::outlineColourId, edge);
        setColour (juce::ComboBox::arrowColourId, dark);
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xffe8dfcc));
        setColour (juce::PopupMenu::textColourId, dark);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xffb8ad9a));
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::black);
        setColour (juce::TextButton::textColourOffId, dark);
        setColour (juce::TextButton::textColourOnId, juce::Colour (0xfff2eadb));
    }

    void drawRotarySlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPos,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider&) override
    {
        auto bounds = juce::Rectangle<float> (static_cast<float> (x),
                                               static_cast<float> (y),
                                               static_cast<float> (width),
                                               static_cast<float> (height));

        const float diameter = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.66f;
        auto knob = bounds.withSizeKeepingCentre (diameter, diameter);
        const auto centre = knob.getCentre();
        const float radius = knob.getWidth() * 0.5f;
        const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        g.setColour (juce::Colour (0xff171714).withAlpha (0.72f));
        for (int i = 0; i <= 10; ++i)
        {
            const float a = rotaryStartAngle
                          + static_cast<float> (i) / 10.0f * (rotaryEndAngle - rotaryStartAngle);
            const float c = std::cos (a);
            const float s = std::sin (a);
            const float r1 = radius * 1.03f;
            const float r2 = radius * ((i == 0 || i == 5 || i == 10) ? 1.23f : 1.16f);

            g.drawLine (centre.x + c * r1,
                        centre.y + s * r1,
                        centre.x + c * r2,
                        centre.y + s * r2,
                        (i == 0 || i == 5 || i == 10) ? 1.5f : 0.9f);
        }

        g.setColour (juce::Colours::black.withAlpha (0.32f));
        g.fillEllipse (knob.translated (2.0f, 3.5f));

        juce::ColourGradient body (juce::Colour (0xff4a4944), knob.getX(), knob.getY(),
                                   juce::Colour (0xff121210), knob.getRight(), knob.getBottom(), false);
        body.addColour (0.42, juce::Colour (0xff2d2c28));
        g.setGradientFill (body);
        g.fillEllipse (knob);

        g.setColour (juce::Colour (0xff080807));
        g.drawEllipse (knob, 1.6f);

        // Bakelite-style radial ribs.
        g.setColour (juce::Colour (0xff62605a).withAlpha (0.34f));
        for (int i = 0; i < 18; ++i)
        {
            const float a = static_cast<float> (i) / 18.0f * juce::MathConstants<float>::twoPi;
            const float c = std::cos (a);
            const float s = std::sin (a);
            g.drawLine (centre.x + c * radius * 0.79f,
                        centre.y + s * radius * 0.79f,
                        centre.x + c * radius * 0.94f,
                        centre.y + s * radius * 0.94f,
                        0.8f);
        }

        auto cap = knob.reduced (radius * 0.19f);
        g.setColour (juce::Colour (0xff24231f));
        g.fillEllipse (cap);
        g.setColour (juce::Colour (0xff595750));
        g.drawEllipse (cap, 0.9f);

        juce::Path pointer;
        pointer.addRectangle (-1.6f, -radius * 0.72f, 3.2f, radius * 0.42f);
        pointer.applyTransform (juce::AffineTransform::rotation (angle)
                                    .translated (centre.x, centre.y));

        g.setColour (juce::Colour (0xffeee7d8));
        g.fillPath (pointer);
    }

    void drawButtonBackground (juce::Graphics& g,
                               juce::Button& button,
                               const juce::Colour&,
                               bool isHighlighted,
                               bool isDown) override
    {
        auto r = button.getLocalBounds().toFloat().reduced (0.8f);
        const bool on = button.getToggleState();

        auto face = on ? juce::Colour (0xff20201d) : juce::Colour (0xffc7bdab);
        if (isHighlighted) face = face.brighter (0.05f);
        if (isDown) face = face.darker (0.12f);

        g.setColour (juce::Colours::black.withAlpha (0.28f));
        g.fillRect (r.translated (1.8f, 2.6f));

        g.setColour (face);
        g.fillRect (r);

        g.setColour (juce::Colour (0xff766d5e));
        g.drawRect (r, 1.2f);

        const float lampX = r.getX() + 10.0f;
        const float lampY = r.getY() + 8.0f;
        g.setColour (on ? juce::Colour (0xffffbf3f) : juce::Colour (0xff777166));
        g.fillEllipse (lampX, lampY, 6.5f, 6.5f);
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.drawEllipse (lampX, lampY, 6.5f, 6.5f, 0.8f);
    }

    void drawComboBox (juce::Graphics& g,
                       int width, int height,
                       bool,
                       int, int, int, int,
                       juce::ComboBox& box) override
    {
        auto r = juce::Rectangle<float> (0.0f, 0.0f,
                                         static_cast<float> (width),
                                         static_cast<float> (height));

        g.setColour (juce::Colours::black.withAlpha (0.20f));
        g.fillRect (r.translated (1.0f, 2.0f));

        g.setColour (juce::Colour (0xffded5c3));
        g.fillRect (r);

        g.setColour (juce::Colour (0xff8c8170));
        g.drawRect (r, 1.0f);

        juce::Path arrow;
        arrow.addTriangle (static_cast<float> (width - 18), static_cast<float> (height) * 0.41f,
                           static_cast<float> (width - 8),  static_cast<float> (height) * 0.41f,
                           static_cast<float> (width - 13), static_cast<float> (height) * 0.64f);
        g.setColour (box.findColour (juce::ComboBox::arrowColourId));
        g.fillPath (arrow);
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override
    {
        return juce::Font (juce::FontOptions (11.0f).withStyle ("Bold"));
    }
};
