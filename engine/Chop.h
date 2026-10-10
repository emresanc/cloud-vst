#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace basement
{

enum class ChopStyle
{
    Flip,
    Stutter,
    Slowed,
    Bounce,
    Original,
    Count
};

struct ChopStyleDef
{
    std::string name;
    std::string description;
    int motifBars;
    std::vector<int> lengths;          // chop lengths in 16th notes
    std::vector<double> lengthWeights;
    double restChance;                 // chance of a gap before a chop
    double continueProb;               // play on from where the last chop ended
    double repeatProb;                 // hit the same chop again
    double jumpProb;                   // jump to one of the pattern's anchor chops
    double rollChance;                 // turn a chop into a 1/16 or 1/32 roll
    double octaveChance;               // +/-12 on single chops (scaled by chaos)
    double reverseChance;              // scaled by chaos
    int semitoneOffset;                // applied to every chop
    int numAnchors;
    std::vector<std::string> forms;    // motif repetition, e.g. "AABA'"
};

const ChopStyleDef& getChopStyle (ChopStyle s);
int numChopStyles();

struct ChopEvent
{
    double start;   // beats
    double length;  // beats (gate)
    int slice;
    int semitones;
    bool reverse;
    float gain;
};

struct ChopPattern
{
    int bars = 4;
    std::vector<ChopEvent> events;
    double lengthBeats() const { return bars * 4.0; }
};

struct ChopSettings
{
    ChopStyle style = ChopStyle::Flip;
    int bars = 4;
    int numSlices = 16;
    int stepsPerSlice = 2;  // how many 16ths one slice spans at the sample's own tempo
    double density = 0.5;
    double chaos = 0.3;
    uint32_t seed = 1;
};

// Deterministic: same settings, same chops.
ChopPattern generateChops (const ChopSettings& settings);

} // namespace basement
