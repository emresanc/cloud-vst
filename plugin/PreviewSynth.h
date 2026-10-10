#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

// Tiny built-in synth so the generated pattern can be heard right away
// (bell for the melody, soft pad for chords, sine 808 for bass). It is only a
// preview: in a real session route the MIDI out / dragged MIDI to your own sounds.
class PreviewSynth
{
public:
    PreviewSynth();

    void prepare (double sampleRate);
    void render (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi);
    void allNotesOff();

private:
    juce::Synthesiser synth;
};
