// Headless tests of the chop plugin: plays a synthetic sample through the
// real processor and checks it makes sound, stays finite and does not click
// at chop boundaries.

#include "../plugin/PluginProcessor.h"

#include <cstdio>
#include <cstdlib>

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

// Smooth sine "keys" so any discontinuity comes from the chopper, not the sample.
static std::shared_ptr<SampleAudio> sineLoop (double sr)
{
    const int len = (int) (16 * 60.0 / 90.0 * sr); // 4 bars @ 90
    juce::AudioBuffer<float> audio (2, len);
    double phase = 0;
    for (int i = 0; i < len; ++i)
    {
        const double f = 220.0 * std::pow (2.0, (double) (((int64_t) i * 8 / len) % 5) / 12.0);
        phase += 2.0 * juce::MathConstants<double>::pi * f / sr; // continuous phase: no clicks in the source
        const float v = 0.8f * (float) std::sin (phase);
        audio.setSample (0, i, v);
        audio.setSample (1, i, v);
    }
    return makeSampleAudio (std::move (audio), sr, "sine");
}

static void setParam (BasementProcessor& p, const char* id, float value)
{
    auto* param = p.apvts.getParameter (id);
    param->setValueNotifyingHost (param->convertTo0to1 (value));
}

static void run (int style, float chaos, int blockSize)
{
    BasementProcessor proc;
    proc.setPlayConfigDetails (0, 2, 44100.0, blockSize);
    setParam (proc, "style", (float) style);
    setParam (proc, "chaos", chaos);
    proc.setSample (sineLoop (44100.0));
    proc.prepareToPlay (44100.0, blockSize);
    proc.setInternalPlaying (true);

    auto state = proc.getState();
    CHECK (state->slices.numSlices() == 32); // 4 bars of 1/8 chops
    CHECK (! state->pattern.events.empty());

    juce::AudioBuffer<float> audio (2, blockSize);
    juce::MidiBuffer midi;
    float peak = 0, maxStep = 0, prev = 0;
    bool finite = true;
    const int blocks = (int) (44100.0 * 8.0 / blockSize);

    for (int b = 0; b < blocks; ++b)
    {
        proc.processBlock (audio, midi);
        const float* d = audio.getReadPointer (0);
        for (int i = 0; i < blockSize; ++i)
        {
            finite = finite && std::isfinite (d[i]);
            peak = std::max (peak, std::abs (d[i]));
            maxStep = std::max (maxStep, std::abs (d[i] - prev));
            prev = d[i];
        }
    }

    CHECK (finite);
    CHECK (peak > 0.1f);
    CHECK (peak < 1.5f);
    // A 330 Hz sine at 0.8 moves at most ~0.04 per sample; a hard cut would jump ~0.8.
    // With octave-up chops (chaos) the sine itself is twice as steep.
    const float limit = chaos > 0.0f ? 0.2f : 0.12f;
    CHECK (maxStep < limit);
    std::printf ("style %d chaos %.1f block %d: peak %.2f, max step %.3f\n", style, chaos, blockSize, peak, maxStep);
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juce;

    for (int style = 0; style < basement::numChopStyles(); ++style)
        run (style, 0.0f, 512);
    run (1, 1.0f, 64);
    run (0, 0.5f, 441);

    // No sample: silent, no crash.
    {
        BasementProcessor proc;
        proc.prepareToPlay (44100.0, 256);
        proc.setInternalPlaying (true);
        juce::AudioBuffer<float> audio (2, 256);
        juce::MidiBuffer midi;
        proc.processBlock (audio, midi);
        CHECK (audio.getMagnitude (0, 256) < 1e-9f);
    }

    // MIDI key plays a chop when stopped.
    {
        BasementProcessor proc;
        proc.setSample (sineLoop (44100.0));
        proc.prepareToPlay (44100.0, 256);
        juce::AudioBuffer<float> audio (2, 256);
        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::noteOn (1, BasementProcessor::firstSliceNote + 3, 1.0f), 0);
        proc.processBlock (audio, midi);
        CHECK (audio.getMagnitude (0, 256) > 0.1f);
    }

    // Export renders exactly one loop.
    {
        BasementProcessor proc;
        proc.setSample (sineLoop (44100.0));
        auto file = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("basement_test.wav");
        CHECK (proc.renderToFile (file));
        juce::AudioFormatManager fm;
        fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (file));
        CHECK (r != nullptr);
        if (r != nullptr)
        {
            const double expected = proc.getState()->pattern.lengthBeats() * 60.0 / proc.getTempo() * 44100.0;
            CHECK (std::abs ((double) r->lengthInSamples - expected) < 2.0);
        }
        file.deleteFile();
    }

    // Optional UI snapshot for docs: BASEMENT_SNAPSHOT=out.png (needs a display).
    if (auto* snap = std::getenv ("BASEMENT_SNAPSHOT"))
    {
        BasementProcessor proc;
        proc.setSample (sineLoop (44100.0));
        std::unique_ptr<juce::AudioProcessorEditor> editor (proc.createEditor());
        auto image = editor->createComponentSnapshot (editor->getLocalBounds());
        juce::File out (juce::File::getCurrentWorkingDirectory().getChildFile (snap));
        out.deleteFile();
        if (auto stream = out.createOutputStream())
            juce::PNGImageFormat().writeImageToStream (image, *stream);
    }

    if (failures == 0)
        std::printf ("all processor tests passed\n");
    return failures == 0 ? 0 : 1;
}
