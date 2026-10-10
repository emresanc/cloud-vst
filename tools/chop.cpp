// Offline chopper: renders every chop style for a sample to .wav, so styles
// can be auditioned (and tuned) without a DAW.
//
//   basement-chop <sample.wav|synthetic> <out-dir> [seed] [bpm]

#include "../plugin/ChopSampler.h"

#include <cstdio>

static std::shared_ptr<SampleAudio> syntheticSample()
{
    // 4 bars at 90 bpm of soft sine "keys": smooth enough to expose clicks.
    const double sr = 44100.0, bpm = 90.0;
    const int len = (int) (16 * 60.0 / bpm * sr);
    juce::AudioBuffer<float> audio (2, len);
    audio.clear();
    const int notes[] = { 57, 60, 64, 67, 65, 64, 62, 60, 57, 60, 64, 69, 67, 64, 62, 59 };
    const int noteLen = len / 16;
    for (int n = 0; n < 16; ++n)
    {
        const double f = 440.0 * std::pow (2.0, (notes[n] - 69) / 12.0);
        for (int i = 0; i < noteLen; ++i)
        {
            const double t = i / sr;
            const float env = (float) (std::min (1.0, t / 0.005) * std::min (1.0, (noteLen - i) / (0.005 * sr)) * std::exp (-t * 2.0));
            const float v = 0.6f * env * (float) std::sin (2.0 * juce::MathConstants<double>::pi * f * t);
            audio.setSample (0, n * noteLen + i, v);
            audio.setSample (1, n * noteLen + i, v);
        }
    }
    return makeSampleAudio (std::move (audio), sr, "synthetic");
}

int main (int argc, char** argv)
{
    if (argc < 3)
    {
        std::printf ("usage: basement-chop <sample|synthetic> <out-dir> [seed] [bpm]\n");
        return 1;
    }

    std::shared_ptr<SampleAudio> sample;
    if (juce::String (argv[1]) == "synthetic")
        sample = syntheticSample();
    else
    {
        juce::String error;
        sample = loadSampleFile (juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]), error);
        if (sample == nullptr)
        {
            std::printf ("%s\n", error.toRawUTF8());
            return 1;
        }
    }

    const juce::File outDir = juce::File::getCurrentWorkingDirectory().getChildFile (argv[2]);
    outDir.createDirectory();
    const uint32_t seed = argc > 3 ? (uint32_t) std::atoi (argv[3]) : 1;

    ChopState state;
    state.sample = sample;
    state.slices = sliceSample (*sample, 0, ChopSize::Eighth);
    const double bpm = argc > 4 ? std::atof (argv[4]) : state.slices.sampleBpm;
    std::printf ("%s: %.1fs, %d bars ~%.0f bpm, %d chops, rendering at %.0f bpm\n", sample->name.toRawUTF8(), sample->getSeconds(),
                 state.slices.sampleBars, state.slices.sampleBpm, state.slices.numSlices(), bpm);

    const double sr = 44100.0;
    for (int st = 0; st < basement::numChopStyles(); ++st)
    {
        basement::ChopSettings cs;
        cs.style = (basement::ChopStyle) st;
        cs.bars = 4;
        cs.numSlices = state.slices.numSlices();
        cs.stepsPerSlice = state.slices.stepsPerSlice;
        cs.seed = seed;
        state.pattern = basement::generateChops (cs);

        const int total = (int) (2 * state.pattern.lengthBeats() * 60.0 / bpm * sr);
        juce::AudioBuffer<float> out (2, total);
        out.clear();
        ChopSampler sampler;
        sampler.prepare (sr);
        const double bps = bpm / 60.0 / sr;
        for (int done = 0; done < total; done += 512)
            sampler.processPattern (out, done, std::min (512, total - done), done * bps, bps, state, 0.0f);

        const auto& def = basement::getChopStyle (cs.style);
        auto file = outDir.getChildFile (juce::String (st + 1) + "_" + def.name + ".wav");
        file.deleteFile();
        juce::WavAudioFormat wav;
        if (auto stream = file.createOutputStream())
            if (auto* w = wav.createWriterFor (stream.get(), sr, 2, 16, {}, 0))
            {
                stream.release();
                std::unique_ptr<juce::AudioFormatWriter> writer (w);
                writer->writeFromAudioSampleBuffer (out, 0, total);
            }
        std::printf ("  %-9s %3zu chops -> %s\n", def.name.c_str(), state.pattern.events.size(), file.getFileName().toRawUTF8());
    }
    return 0;
}
