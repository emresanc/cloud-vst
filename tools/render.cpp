// Renders every style through the preview synth to .wav (+ the .mid) so the
// styles can be auditioned without installing the plugin.
//
//   basement-render <out-dir> [seed] [key]

#include "../plugin/PreviewSynth.h"
#include "Generator.h"
#include "MidiWriter.h"

#include <juce_audio_formats/juce_audio_formats.h>

int main (int argc, char** argv)
{
    using namespace basement;

    const juce::File outDir (juce::File::getCurrentWorkingDirectory().getChildFile (argc > 1 ? argv[1] : "previews"));
    const uint32_t seed = argc > 2 ? (uint32_t) std::atoi (argv[2]) : 1;
    const int key = argc > 3 ? parseNoteName (argv[3]) : 0;
    outDir.createDirectory();

    const double sr = 44100.0;
    const int loops = 2;

    for (int i = 0; i < numStyles(); ++i)
    {
        Settings s;
        s.style = (StyleId) i;
        s.seed = seed;
        s.rootPc = std::max (0, key);
        const auto& style = getStyle (s.style);
        const auto p = generate (s);
        const double bpm = style.defaultTempo;
        const double samplesPerBeat = sr * 60.0 / bpm;
        const int total = (int) (loops * p.lengthBeats() * samplesPerBeat + sr * 1.5);

        juce::MidiBuffer midi;
        for (int l = 0; l < loops; ++l)
            for (auto& n : p.notes)
            {
                const double offset = l * p.lengthBeats();
                midi.addEvent (juce::MidiMessage::noteOn (n.channel, n.pitch, (juce::uint8) n.velocity), (int) ((offset + n.start) * samplesPerBeat));
                midi.addEvent (juce::MidiMessage::noteOff (n.channel, n.pitch), (int) ((offset + n.start + n.length) * samplesPerBeat));
            }

        PreviewSynth synth;
        synth.prepare (sr);
        juce::AudioBuffer<float> audio (2, total);
        audio.clear();
        synth.render (audio, midi);
        audio.applyGain (0.9f / std::max (0.01f, audio.getMagnitude (0, total)));

        auto name = juce::String (i + 1).paddedLeft ('0', 2) + "_" + juce::String (style.name).removeCharacters (" /");
        auto wavFile = outDir.getChildFile (name + ".wav");
        wavFile.deleteFile();
        juce::WavAudioFormat wav;
        if (auto stream = wavFile.createOutputStream())
            if (auto writer = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (stream.get(), sr, 2, 16, {}, 0)))
            {
                stream.release();
                writer->writeFromAudioSampleBuffer (audio, 0, total);
            }

        saveMidiFile (outDir.getChildFile (name + ".mid").getFullPathName().toStdString(), p, bpm);
        std::printf ("%-16s %s\n", style.name.c_str(), p.chordSummary().c_str());
    }
    return 0;
}
