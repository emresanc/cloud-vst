#pragma once

#include "Chop.h"

#include <juce_audio_formats/juce_audio_formats.h>

#include <memory>
#include <vector>

// The loaded audio. Immutable once created, shared between threads.
struct SampleAudio
{
    juce::AudioBuffer<float> audio;
    double sampleRate = 44100.0;
    juce::String name;
    juce::File file;
    std::vector<float> peaks; // max |x| per display bucket

    int getLength() const { return audio.getNumSamples(); }
    double getSeconds() const { return getLength() / sampleRate; }
};

enum class ChopSize
{
    Quarter,
    Eighth,
    Sixteenth,
    Transient
};

// Where each chop starts, plus the tempo assumptions behind it.
struct SliceMap
{
    std::vector<int> starts;
    int stepsPerSlice = 2;
    int sampleBars = 1;
    double sampleBpm = 120.0;

    int numSlices() const { return (int) starts.size(); }
    int sliceEnd (int i, int totalLength) const
    {
        return i + 1 < numSlices() ? starts[(size_t) i + 1] : totalLength;
    }
};

std::shared_ptr<SampleAudio> loadSampleFile (const juce::File& file, juce::String& error);
std::shared_ptr<SampleAudio> makeSampleAudio (juce::AudioBuffer<float> audio, double sampleRate, const juce::String& name);

// sampleBars <= 0 means guess from the length (assumes the sample is a loop).
SliceMap sliceSample (const SampleAudio& sample, int sampleBars, ChopSize size);

int guessSampleBars (double seconds);
