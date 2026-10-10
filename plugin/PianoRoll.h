#pragma once

#include "Generator.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>

// Read-only view of the generated pattern: notes per layer, chord names and
// a moving playhead.
class PianoRoll : public juce::Component
{
public:
    void setPattern (std::shared_ptr<const basement::Pattern> p);
    void setPlayhead (double beat);

    void paint (juce::Graphics& g) override;

private:
    std::shared_ptr<const basement::Pattern> pattern;
    double playhead = -1.0;
};
