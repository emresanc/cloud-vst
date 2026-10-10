#pragma once

#include "SampleData.h"

#include <array>

// Everything the audio thread needs to play a pattern.
struct ChopState
{
    std::shared_ptr<const SampleAudio> sample;
    SliceMap slices;
    basement::ChopPattern pattern;
};

// Plays chops from a sample. Each new chop chokes the previous one with a
// short crossfade (like an MPC mono pad), so there are no clicks at the cuts.
// Pitch works by playback speed, the classic sampler way.
class ChopSampler
{
public:
    void prepare (double sampleRate);
    void reset();

    // Renders [start, start + num) of `out`, firing the pattern's chops that
    // fall between startBeat and startBeat + num * beatsPerSample (looped).
    // Returns the last slice triggered in this block, or -1.
    int processPattern (juce::AudioBuffer<float>& out, int start, int num, double startBeat, double beatsPerSample,
                        const ChopState& state, float pitchSemis);

    // Manual playing (MIDI): gate stays open until releaseNote.
    void triggerManual (const ChopState& state, int slice, float pitchSemis, float gain, int note);
    void releaseNote (int note);
    void render (juce::AudioBuffer<float>& out, int start, int num, const ChopState& state);
    void releaseAll();

private:
    struct Voice
    {
        bool active = false;
        double pos = 0, inc = 1, endPos = 0;
        int dir = 1;
        float gain = 1;
        int64_t gateLeft = 0;   // samples until release; < 0 = held
        int attack = 0, attackLen = 1;
        int release = 0, releaseLen = 1;
        bool releasing = false;
        int note = -1;
    };

    void trigger (const ChopState& state, const basement::ChopEvent& e, double gateSamples, float pitchSemis, int note);
    void startRelease (Voice& v, double seconds);

    std::array<Voice, 8> voices;
    double sampleRate = 44100.0;

    struct Trigger
    {
        int offset;
        size_t event;
    };
    std::vector<Trigger> triggers;
};
