#include "PianoRoll.h"
#include "Theme.h"

void PianoRoll::setPattern (std::shared_ptr<const basement::Pattern> p)
{
    pattern = std::move (p);
    repaint();
}

void PianoRoll::setPlayhead (double beat)
{
    if (std::abs (beat - playhead) > 1e-4)
    {
        playhead = beat;
        repaint();
    }
}

void PianoRoll::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    g.setColour (theme::panel);
    g.fillRoundedRectangle (area, 6.0f);

    if (pattern == nullptr || pattern->notes.empty())
        return;

    const float headerH = 20.0f;
    auto header = area.removeFromTop (headerH);
    area.reduce (6.0f, 4.0f);

    int lo = 127, hi = 0;
    for (auto& n : pattern->notes)
    {
        lo = std::min (lo, n.pitch);
        hi = std::max (hi, n.pitch);
    }
    lo -= 1;
    hi += 1;

    const double beats = pattern->lengthBeats();
    const float rowH = area.getHeight() / (float) (hi - lo + 1);
    auto xFor = [&] (double beat) { return area.getX() + (float) (beat / beats) * area.getWidth(); };
    auto yFor = [&] (int pitch) { return area.getY() + (float) (hi - pitch) * rowH; };

    // Black-key rows
    for (int p = lo; p <= hi; ++p)
    {
        const int pc = p % 12;
        if (pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10)
        {
            g.setColour (theme::background.withAlpha (0.35f));
            g.fillRect (area.getX(), yFor (p), area.getWidth(), rowH);
        }
    }

    // Grid
    for (int b = 0; b <= (int) beats; ++b)
    {
        const bool bar = b % pattern->beatsPerBar == 0;
        g.setColour (bar ? theme::grid.brighter (0.4f) : theme::grid);
        g.drawVerticalLine (juce::roundToInt (xFor (b)), area.getY(), area.getBottom());
    }

    // Chord names
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    for (auto& c : pattern->chords)
    {
        auto r = juce::Rectangle<float> (xFor (c.start), header.getY(), xFor (c.start + c.length) - xFor (c.start), headerH).reduced (6.0f, 0.0f);
        g.setColour (theme::textDim);
        g.drawText (c.name, r, juce::Justification::centredLeft);
    }

    // Notes: chords and bass first, melody on top.
    for (int pass : { basement::ChordChannel, basement::BassChannel, basement::MelodyChannel })
    {
        const auto colour = theme::layerColour (pass);
        for (auto& n : pattern->notes)
        {
            if (n.channel != pass)
                continue;
            auto r = juce::Rectangle<float> (xFor (n.start), yFor (n.pitch), std::max (2.0f, xFor (n.start + n.length) - xFor (n.start)), rowH)
                         .reduced (0.5f, rowH > 4.0f ? 0.5f : 0.0f);
            g.setColour (colour.withAlpha (0.45f + 0.55f * (float) n.velocity / 127.0f));
            g.fillRoundedRectangle (r, 1.5f);
        }
    }

    if (playhead >= 0.0)
    {
        g.setColour (theme::text.withAlpha (0.8f));
        g.drawVerticalLine (juce::roundToInt (xFor (playhead)), header.getY(), area.getBottom());
    }
}
