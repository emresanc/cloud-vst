#include "SampleData.h"

namespace
{

constexpr double maxSeconds = 180.0;
constexpr int numPeaks = 2048;

void computePeaks (SampleAudio& s)
{
    s.peaks.assign (numPeaks, 0.0f);
    const int len = s.getLength();
    if (len == 0)
        return;

    for (int ch = 0; ch < s.audio.getNumChannels(); ++ch)
    {
        const float* d = s.audio.getReadPointer (ch);
        for (int i = 0; i < len; ++i)
        {
            auto& p = s.peaks[(size_t) ((int64_t) i * numPeaks / len)];
            p = std::max (p, std::abs (d[i]));
        }
    }
}

std::vector<int> detectTransients (const SampleAudio& s)
{
    const int hop = 256, win = 1024;
    const int len = s.getLength();
    const int frames = len / hop;
    std::vector<float> energy ((size_t) std::max (0, frames), 0.0f);

    for (int f = 0; f < frames; ++f)
    {
        double e = 0;
        for (int ch = 0; ch < s.audio.getNumChannels(); ++ch)
        {
            const float* d = s.audio.getReadPointer (ch);
            for (int i = f * hop; i < std::min (len, f * hop + win); ++i)
                e += d[i] * d[i];
        }
        energy[(size_t) f] = (float) std::log (1e-6 + e);
    }

    std::vector<float> flux (energy.size(), 0.0f);
    for (size_t f = 1; f < energy.size(); ++f)
        flux[f] = std::max (0.0f, energy[f] - energy[f - 1]);

    std::vector<int> starts { 0 };
    const int minGap = (int) (0.09 * s.sampleRate / hop);
    const int radius = 16;

    for (int f = 1; f + 1 < (int) flux.size(); ++f)
    {
        if (flux[(size_t) f] < flux[(size_t) f - 1] || flux[(size_t) f] < flux[(size_t) f + 1])
            continue;

        double mean = 0, sq = 0;
        int n = 0;
        for (int k = std::max (0, f - radius); k < std::min ((int) flux.size(), f + radius); ++k, ++n)
        {
            mean += flux[(size_t) k];
            sq += flux[(size_t) k] * flux[(size_t) k];
        }
        mean /= n;
        const double sd = std::sqrt (std::max (0.0, sq / n - mean * mean));

        if (flux[(size_t) f] > mean + 1.5 * sd && flux[(size_t) f] > 0.5
            && f * hop - starts.back() > minGap * hop)
            starts.push_back (f * hop);

        if (starts.size() >= 64)
            break;
    }
    return starts;
}

} // namespace

int guessSampleBars (double seconds)
{
    // Most samples are loops: pick the bar count that puts the loop at a
    // believable tempo, preferring something near 120 (so a 90 bpm loop is
    // read as 90, not 180; Sample Bars overrides this when it guesses wrong).
    int best = 1;
    double bestScore = 1e9;
    for (int bars : { 1, 2, 4, 8, 16 })
    {
        const double bpm = bars * 4 * 60.0 / std::max (0.1, seconds);
        if (bpm < 55 || bpm > 200)
            continue;
        const double score = std::abs (std::log (bpm / 120.0));
        if (score < bestScore)
        {
            bestScore = score;
            best = bars;
        }
    }
    return best;
}

std::shared_ptr<SampleAudio> makeSampleAudio (juce::AudioBuffer<float> audio, double sampleRate, const juce::String& name)
{
    auto s = std::make_shared<SampleAudio>();
    s->audio = std::move (audio);
    s->sampleRate = sampleRate;
    s->name = name;
    computePeaks (*s);
    return s;
}

std::shared_ptr<SampleAudio> loadSampleFile (const juce::File& file, juce::String& error)
{
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));
    if (reader == nullptr)
    {
        error = "Bu dosya okunamadi: " + file.getFileName();
        return nullptr;
    }

    const auto length = (int) std::min<juce::int64> (reader->lengthInSamples, (juce::int64) (maxSeconds * reader->sampleRate));
    if (length <= 0)
    {
        error = "Dosya bos: " + file.getFileName();
        return nullptr;
    }

    const int channels = (int) juce::jlimit (1u, 2u, reader->numChannels);
    juce::AudioBuffer<float> audio (channels, length);
    reader->read (&audio, 0, length, 0, true, channels > 1);

    // Normalise so quiet samples still hit.
    const float mag = audio.getMagnitude (0, length);
    if (mag > 0.0001f)
        audio.applyGain (0.9f / mag);

    auto s = makeSampleAudio (std::move (audio), reader->sampleRate, file.getFileNameWithoutExtension());
    s->file = file;
    return s;
}

SliceMap sliceSample (const SampleAudio& sample, int sampleBars, ChopSize size)
{
    SliceMap map;
    const int len = sample.getLength();
    map.sampleBars = sampleBars > 0 ? sampleBars : guessSampleBars (sample.getSeconds());
    map.sampleBpm = map.sampleBars * 4 * 60.0 / std::max (0.01, sample.getSeconds());

    if (len <= 0)
    {
        map.starts = { 0 };
        return map;
    }

    if (size == ChopSize::Transient)
    {
        map.starts = detectTransients (sample);
        map.stepsPerSlice = 2;
        return map;
    }

    map.stepsPerSlice = size == ChopSize::Quarter ? 4 : size == ChopSize::Eighth ? 2 : 1;
    const int count = std::clamp (map.sampleBars * 16 / map.stepsPerSlice, 1, 256);
    for (int i = 0; i < count; ++i)
        map.starts.push_back ((int) ((int64_t) i * len / count));
    return map;
}
