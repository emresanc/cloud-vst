#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace theme
{

inline const juce::Colour background { 0xff0d0d10 };
inline const juce::Colour panel { 0xff17171c };
inline const juce::Colour grid { 0xff24242c };
inline const juce::Colour text { 0xffe8e6e3 };
inline const juce::Colour textDim { 0xff8a8790 };
inline const juce::Colour accent { 0xffe0303c };   // blood red

// Distinct, muted colours so the same chop is recognisable in the waveform and the pattern.
inline juce::Colour sliceColour (int slice)
{
    const float hue = std::fmod (0.97f + 0.13f * (float) slice, 1.0f);
    return juce::Colour::fromHSV (hue, 0.55f, 0.85f, 1.0f);
}

class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    LookAndFeel()
    {
        setColour (juce::ResizableWindow::backgroundColourId, background);
        setColour (juce::ComboBox::backgroundColourId, panel);
        setColour (juce::ComboBox::outlineColourId, grid);
        setColour (juce::ComboBox::textColourId, text);
        setColour (juce::ComboBox::arrowColourId, textDim);
        setColour (juce::PopupMenu::backgroundColourId, panel);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, accent.withAlpha (0.6f));
        setColour (juce::PopupMenu::textColourId, text);
        setColour (juce::Label::textColourId, text);
        setColour (juce::TextButton::buttonColourId, panel);
        setColour (juce::TextButton::buttonOnColourId, accent);
        setColour (juce::TextButton::textColourOffId, text);
        setColour (juce::TextButton::textColourOnId, text);
        setColour (juce::ToggleButton::textColourId, text);
        setColour (juce::ToggleButton::tickColourId, accent);
        setColour (juce::ToggleButton::tickDisabledColourId, textDim);
        setColour (juce::Slider::textBoxTextColourId, textDim);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override
    {
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
        const float radius = std::min (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto c = bounds.getCentre();
        const float angle = startAngle + pos * (endAngle - startAngle);
        const float stroke = 4.0f;

        juce::Path track, value;
        track.addCentredArc (c.x, c.y, radius - stroke, radius - stroke, 0.0f, startAngle, endAngle, true);
        value.addCentredArc (c.x, c.y, radius - stroke, radius - stroke, 0.0f, startAngle, angle, true);

        g.setColour (grid);
        g.strokePath (track, juce::PathStrokeType (stroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (accent);
        g.strokePath (value, juce::PathStrokeType (stroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        const float inner = radius - stroke * 2.5f;
        g.setColour (panel.brighter (0.15f));
        g.fillEllipse (c.x - inner, c.y - inner, inner * 2.0f, inner * 2.0f);

        juce::Path tick;
        tick.addRoundedRectangle (-1.5f, -inner, 3.0f, inner * 0.45f, 1.5f);
        g.setColour (text);
        g.fillPath (tick, juce::AffineTransform::rotation (angle).translated (c.x, c.y));
    }
};

} // namespace theme
