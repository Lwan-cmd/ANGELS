#pragma once

#include <JuceHeader.h>

class AngelAnalogLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    AngelAnalogLookAndFeel()
    {
        setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff34342f));
        setColour (juce::ComboBox::textColourId, juce::Colour (0xffe7dfc9));
        setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff1f201d));
        setColour (juce::ComboBox::arrowColourId, juce::Colour (0xffd6c9a8));
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff34342f));
        setColour (juce::PopupMenu::textColourId, juce::Colour (0xffeee6d2));
        setColour (juce::TextButton::textColourOffId, juce::Colour (0xffddd3b9));
        setColour (juce::TextButton::textColourOnId, juce::Colour (0xff211f1b));
    }

    void drawRotarySlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPos,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider&) override
    {
        auto bounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                               static_cast<float> (width), static_cast<float> (height));
        const float diameter = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.72f;
        auto knob = bounds.withSizeKeepingCentre (diameter, diameter);
        const auto centre = knob.getCentre();
        const float radius = knob.getWidth() * 0.5f;
        const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // Printed scale around the pot.
        g.setColour (juce::Colour (0xffd8cfb8).withAlpha (0.65f));
        for (int i = 0; i <= 10; ++i)
        {
            const float a = rotaryStartAngle + static_cast<float> (i) / 10.0f * (rotaryEndAngle - rotaryStartAngle);
            const float c = std::cos (a);
            const float s = std::sin (a);
            const float r1 = radius * 1.02f;
            const float r2 = radius * (i == 0 || i == 5 || i == 10 ? 1.17f : 1.12f);
            g.drawLine (centre.x + c * r1, centre.y + s * r1,
                        centre.x + c * r2, centre.y + s * r2,
                        i == 0 || i == 5 || i == 10 ? 1.5f : 0.8f);
        }

        // Dark bakelite skirt.
        g.setColour (juce::Colours::black.withAlpha (0.28f));
        g.fillEllipse (knob.translated (1.5f, 3.0f));

        g.setColour (juce::Colour (0xff171816));
        g.fillEllipse (knob);
        g.setColour (juce::Colour (0xff090a09));
        g.drawEllipse (knob, 1.4f);

        auto cap = knob.reduced (radius * 0.18f);
        juce::ColourGradient capGradient (juce::Colour (0xff67675e), cap.getX(), cap.getY(),
                                          juce::Colour (0xff292a27), cap.getRight(), cap.getBottom(), false);
        capGradient.addColour (0.52, juce::Colour (0xff4c4d47));
        g.setGradientFill (capGradient);
        g.fillEllipse (cap);
        g.setColour (juce::Colour (0xff77766c).withAlpha (0.7f));
        g.drawEllipse (cap, 1.0f);

        // Ivory pointer line.
        juce::Path pointer;
        pointer.addRoundedRectangle (-1.7f, -radius * 0.69f,
                                     3.4f, radius * 0.38f, 1.4f);
        pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
        g.setColour (juce::Colour (0xfff0e7d0));
        g.fillPath (pointer);
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                               const juce::Colour&, bool isHighlighted, bool isDown) override
    {
        auto r = button.getLocalBounds().toFloat().reduced (0.5f);
        const bool on = button.getToggleState();
        auto base = on ? juce::Colour (0xffc7b171) : juce::Colour (0xff3c3b35);
        if (isHighlighted) base = base.brighter (0.08f);
        if (isDown) base = base.darker (0.12f);

        g.setColour (juce::Colours::black.withAlpha (0.30f));
        g.fillRoundedRectangle (r.translated (1.0f, 2.0f), 2.0f);
        g.setColour (base);
        g.fillRoundedRectangle (r, 2.0f);
        g.setColour (juce::Colour (0xff181916));
        g.drawRoundedRectangle (r, 2.0f, 1.2f);
    }

    void drawComboBox (juce::Graphics& g, int width, int height,
                       bool, int, int, int, int, juce::ComboBox& box) override
    {
        auto r = juce::Rectangle<float> (0.0f, 0.0f, static_cast<float> (width), static_cast<float> (height));
        g.setColour (juce::Colour (0xff292a27));
        g.fillRoundedRectangle (r, 2.0f);
        g.setColour (juce::Colour (0xff151613));
        g.drawRoundedRectangle (r.reduced (0.5f), 2.0f, 1.0f);

        juce::Path arrow;
        arrow.addTriangle (static_cast<float> (width - 16), static_cast<float> (height) * 0.42f,
                           static_cast<float> (width - 8),  static_cast<float> (height) * 0.42f,
                           static_cast<float> (width - 12), static_cast<float> (height) * 0.62f);
        g.setColour (box.findColour (juce::ComboBox::arrowColourId));
        g.fillPath (arrow);
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override
    {
        return juce::Font (juce::FontOptions (12.0f).withStyle ("Bold"));
    }
};
