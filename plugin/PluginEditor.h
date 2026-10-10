#pragma once

#include "PluginProcessor.h"
#include "Theme.h"

#include <juce_gui_extra/juce_gui_extra.h>

// Sample waveform with slice markers; the chop that is playing lights up.
class WaveformView : public juce::Component
{
public:
    void setState (std::shared_ptr<const ChopState> s);
    void setActiveSlice (int slice);
    void setDropHighlight (bool on);
    void paint (juce::Graphics& g) override;

private:
    std::shared_ptr<const ChopState> state;
    int activeSlice = -1;
    bool dropHighlight = false;
};

// The generated pattern: which chop plays when.
class ChopLane : public juce::Component
{
public:
    void setState (std::shared_ptr<const ChopState> s);
    void setPlayhead (double beat);
    void paint (juce::Graphics& g) override;

private:
    std::shared_ptr<const ChopState> state;
    double playhead = -1.0;
};

// Drag the rendered loop straight into the FL playlist.
class DragChip : public juce::Component
{
public:
    explicit DragChip (BasementProcessor& p);
    void paint (juce::Graphics& g) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent&) override { dragging = false; }

private:
    BasementProcessor& processor;
    bool dragging = false;
};

class BasementEditor : public juce::AudioProcessorEditor,
                       public juce::FileDragAndDropTarget,
                       private juce::Timer
{
public:
    explicit BasementEditor (BasementProcessor&);
    ~BasementEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void fileDragEnter (const juce::StringArray&, int, int) override { waveform.setDropHighlight (true); }
    void fileDragExit (const juce::StringArray&) override { waveform.setDropHighlight (false); }
    void filesDropped (const juce::StringArray& files, int, int) override;

private:
    void timerCallback() override;
    void refreshState();
    void chooseSample();
    void saveWav();
    void loadFile (const juce::File& f);

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

    void setupCombo (Combo&, const juce::String& name, const juce::String& paramId);
    void setupKnob (Knob&, const juce::String& name, const juce::String& paramId);

    BasementProcessor& proc;
    theme::LookAndFeel lnf;

    juce::Label title, subtitle, description, sampleInfo, seedLabel;
    juce::TextButton loadButton { "LOAD SAMPLE" };
    WaveformView waveform;
    Combo style, bars, chop, sampleBars;
    Knob pitch, density, chaos, tone, tempo, volume;
    juce::TextButton generateButton { "CHOP IT" }, prevSeed { "<" }, nextSeed { ">" }, playButton { "PLAY" }, saveButton { "SAVE WAV" };
    ChopLane lane;
    DragChip dragChip;
    juce::Label dragHint;
    std::unique_ptr<juce::FileChooser> chooser;
    int shownVersion = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BasementEditor)
};
