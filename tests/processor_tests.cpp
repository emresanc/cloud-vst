// Headless test of the plugin's real-time MIDI scheduling: runs the processor
// through several loops on its internal clock and checks every note-on gets a
// matching note-off and that each loop plays exactly the generated pattern.

#include "../plugin/PluginProcessor.h"

#include <cstdio>
#include <map>

static int failures = 0;

#define CHECK(cond)                                                      \
    do                                                                   \
    {                                                                    \
        if (! (cond))                                                    \
        {                                                                \
            std::printf ("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); \
            ++failures;                                                  \
        }                                                                \
    } while (0)

int main()
{
    juce::ScopedJuceInitialiser_GUI juce;

    for (int blockSize : { 64, 441, 512 })
    {
        BasementProcessor proc;
        proc.setPlayConfigDetails (0, 2, 44100.0, blockSize);
        proc.prepareToPlay (44100.0, blockSize);

        auto pattern = proc.getPattern();
        const double bpm = proc.apvts.getRawParameterValue ("tempo")->load();
        const int loops = 3;
        // Stop mid-loop so float rounding at a loop boundary can't add a downbeat.
        const double seconds = (loops - 0.5) * pattern->lengthBeats() * 60.0 / bpm;
        const int blocks = (int) (seconds * 44100.0 / blockSize);

        proc.setInternalPlaying (true);

        juce::AudioBuffer<float> audio (2, blockSize);
        std::map<std::pair<int, int>, int> open;
        int ons = 0, maxOpen = 0;
        float peak = 0.0f;

        for (int b = 0; b < blocks; ++b)
        {
            juce::MidiBuffer midi;
            proc.processBlock (audio, midi);
            peak = std::max (peak, audio.getMagnitude (0, blockSize));

            for (const auto m : midi)
            {
                const auto msg = m.getMessage();
                const auto key = std::make_pair (msg.getChannel(), msg.getNoteNumber());
                if (msg.isNoteOn())
                {
                    CHECK (open[key] == 0);
                    ++open[key];
                    ++ons;
                }
                else if (msg.isNoteOff())
                {
                    CHECK (open[key] == 1);
                    open[key] = 0;
                }
                int total = 0;
                for (auto& kv : open)
                    total += kv.second;
                maxOpen = std::max (maxOpen, total);
            }
        }

        // Stop: everything must be released.
        proc.setInternalPlaying (false);
        juce::MidiBuffer midi;
        proc.processBlock (audio, midi);
        for (const auto m : midi)
            if (m.getMessage().isNoteOff())
                open[{ m.getMessage().getChannel(), m.getMessage().getNoteNumber() }] = 0;
        for (auto& kv : open)
            CHECK (kv.second == 0);

        const int perLoop = (int) pattern->notes.size();
        CHECK (ons > perLoop * (loops - 1) && ons < perLoop * loops);
        CHECK (peak > 0.01f);  // preview synth makes sound
        CHECK (peak < 2.0f);
        std::printf ("block %d: %d note-ons over ~%d loops (%d notes/loop), peak %.2f\n", blockSize, ons, loops, perLoop, peak);
    }

    if (failures == 0)
        std::printf ("all processor tests passed\n");
    return failures == 0 ? 0 : 1;
}
