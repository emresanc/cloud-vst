#pragma once

#include "PianoRoll.h"
#include "PluginProcessor.h"
#include "Theme.h"

#include <juce_gui_extra/juce_gui_extra.h>

// A chip you drag straight from the plugin into the DAW's playlist / piano roll.
class DragChip : public juce::Component
{
public:
    DragChip (BasementProcessor& p, juce::String name, int layerMask, juce::Colour c);

    void paint (juce::Graphics& g) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent&) override { dragging = false; }

private:
    BasementProcessor& processor;
    juce::String label;
    int layers;
    juce::Colour colour;
    bool dragging = false;
};

class BasementEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit BasementEditor (BasementProcessor&);
    ~BasementEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void refreshPattern();
    void saveMidi();

    using APVTS = juce::AudioProcessorValueTreeState;

    struct Combo
    {
        juce::Label label;
        juce::ComboBox box;
        std::unique_ptr<APVTS::ComboBoxAttachment> attachment;
    };

    struct Knob
    {
        juce::Label label;
        juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
        std::unique_ptr<APVTS::SliderAttachment> attachment;
    };

    struct Toggle
    {
        juce::ToggleButton button;
        std::unique_ptr<APVTS::ButtonAttachment> attachment;
    };

    void setupCombo (Combo&, const juce::String& name, const juce::String& paramId);
    void setupKnob (Knob&, const juce::String& name, const juce::String& paramId);
    void setupToggle (Toggle&, const juce::String& name, const juce::String& paramId);

    BasementProcessor& proc;
    theme::LookAndFeel lnf;

    juce::Label title, subtitle, description, seedLabel;
    Combo style, key, scale, bars, octave;
    Knob density, darkness, swing, humanize, tempo, volume;
    Toggle melodyToggle, chordsToggle, bassToggle, soundToggle;
    juce::TextButton generateButton { "GENERATE" }, prevSeed { "<" }, nextSeed { ">" }, playButton { "PLAY" }, saveButton { "SAVE MIDI" };
    PianoRoll roll;
    DragChip dragAll, dragMelody, dragChords, dragBass;
    juce::Label dragHint;
    std::unique_ptr<juce::FileChooser> chooser;
    int shownVersion = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BasementEditor)
};
