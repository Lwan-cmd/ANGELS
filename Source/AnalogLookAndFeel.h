#pragma once

#include <JuceHeader.h>
#include <cmath>

class IngeniumLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    IngeniumLookAndFeel()
    {
        setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xfff2eee4));
        setColour (juce::ComboBox::textColourId, juce::Colour (0xff252525));
        setColour (juce::ComboBox::outlineColourId, juce::Colour (0xffb7b0a3));
        setColour (juce::ComboBox::arrowColourId, juce::Colour (0xff353535));
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xfff4f0e8));
        setColour (juce::PopupMenu::textColourId, juce::Colour (0xff252525));
        setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xffdcd6ca));
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colour (0xff111111));
        setColour (juce::TextButton::textColourOffId, juce::Colour (0xff262626));
        setColour (juce::TextButton::textColourOnId, juce::Colour (0xfff5f2e9));
    }

    void drawRotarySlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPos,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider&) override
    {
        auto bounds = juce::Rectangle<float> (
            static_cast<float> (x),
            static_cast<float> (y),
            static_cast<float> (width),
            static_cast<float> (height));

        const float diameter =
            juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.68f;

        auto knob = bounds.withSizeKeepingCentre (diameter, diameter);
        const auto centre = knob.getCentre();
        const float radius = knob.getWidth() * 0.5f;

        const float angle =
            rotaryStartAngle
            + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        g.setColour (juce::Colour (0xff272727).withAlpha (0.70f));

        for (int i = 0; i <= 10; ++i)
        {
            const float a =
                rotaryStartAngle
                + static_cast<float> (i) / 10.0f
                    * (rotaryEndAngle - rotaryStartAngle);

            const float c = std::cos (a);
            const float s = std::sin (a);
            const float r1 = radius * 1.05f;
            const float r2 = radius * (i == 0 || i == 5 || i == 10 ? 1.18f : 1.13f);

            g.drawLine (
                centre.x + c * r1,
                centre.y + s * r1,
                centre.x + c * r2,
                centre.y + s * r2,
                i == 0 || i == 5 || i == 10 ? 1.35f : 0.85f);
        }

        g.setColour (juce::Colours::black.withAlpha (0.20f));
        g.fillEllipse (knob.translated (1.5f, 3.0f));

        juce::ColourGradient bodyGradient (
            juce::Colour (0xff4b4b4b), knob.getX(), knob.getY(),
            juce::Colour (0xff151515), knob.getRight(), knob.getBottom(), false);

        bodyGradient.addColour (0.45, juce::Colour (0xff303030));

        g.setGradientFill (bodyGradient);
        g.fillEllipse (knob);

        g.setColour (juce::Colour (0xff090909));
        g.drawEllipse (knob, 1.3f);

        auto cap = knob.reduced (radius * 0.18f);
        g.setColour (juce::Colour (0xff232323));
        g.fillEllipse (cap);

        g.setColour (juce::Colour (0xff555555));
        g.drawEllipse (cap, 0.8f);

        juce::Path pointer;
        pointer.addRoundedRectangle (
            -1.7f,
            -radius * 0.70f,
            3.4f,
            radius * 0.39f,
            1.4f);

        pointer.applyTransform (
            juce::AffineTransform::rotation (angle)
                .translated (centre.x, centre.y));

        g.setColour (juce::Colour (0xfff1efe7));
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

        auto base = on
            ? juce::Colour (0xff272727)
            : juce::Colour (0xffeee9df);

        if (isHighlighted)
            base = base.brighter (on ? 0.08f : 0.03f);

        if (isDown)
            base = base.darker (0.08f);

        g.setColour (juce::Colours::black.withAlpha (0.14f));
        g.fillRoundedRectangle (r.translated (0.8f, 1.8f), 5.0f);

        g.setColour (base);
        g.fillRoundedRectangle (r, 5.0f);

        g.setColour (juce::Colour (0xffa8a195));
        g.drawRoundedRectangle (r, 5.0f, 1.0f);

        if (on)
        {
            g.setColour (juce::Colour (0xffffc778));
            g.fillEllipse (r.getX() + 9.0f, r.getY() + 8.0f, 6.0f, 6.0f);
        }
        else
        {
            g.setColour (juce::Colour (0xffaaa59b));
            g.fillEllipse (r.getX() + 9.0f, r.getY() + 8.0f, 6.0f, 6.0f);
        }
    }

    void drawComboBox (juce::Graphics& g,
                       int width,
                       int height,
                       bool,
                       int, int, int, int,
                       juce::ComboBox& box) override
    {
        auto r = juce::Rectangle<float> (
            0.0f, 0.0f,
            static_cast<float> (width),
            static_cast<float> (height));

        g.setColour (juce::Colour (0xfff2eee5));
        g.fillRoundedRectangle (r, 5.0f);

        g.setColour (juce::Colour (0xffbdb5a8));
        g.drawRoundedRectangle (r.reduced (0.5f), 5.0f, 1.0f);

        juce::Path arrow;
        arrow.addTriangle (
            static_cast<float> (width - 18), static_cast<float> (height) * 0.43f,
            static_cast<float> (width - 8),  static_cast<float> (height) * 0.43f,
            static_cast<float> (width - 13), static_cast<float> (height) * 0.62f);

        g.setColour (box.findColour (juce::ComboBox::arrowColourId));
        g.fillPath (arrow);
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override
    {
        return juce::Font (
            juce::FontOptions (12.0f).withStyle ("Bold"));
    }
};
