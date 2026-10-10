#pragma once

#include "Styles.h"
#include "Theory.h"

#include <cstdint>
#include <string>
#include <vector>

namespace basement
{

// MIDI channels (1-based) used for the three layers.
enum Channel
{
    MelodyChannel = 1,
    ChordChannel = 2,
    BassChannel = 3
};

struct Note
{
    int pitch;
    double start;  // beats from pattern start
    double length; // beats
    int velocity;
    int channel;
};

struct ChordSlot
{
    double start;
    double length;
    int degree;              // scale degree of the chord root
    std::string name;        // "Cm", "Abmaj7", ...
    std::vector<int> pitches;
};

struct Pattern
{
    int bars = 4;
    int beatsPerBar = 4;
    std::vector<Note> notes;
    std::vector<ChordSlot> chords;

    double lengthBeats() const { return (double) (bars * beatsPerBar); }
    std::string chordSummary() const;
};

struct Settings
{
    StyleId style = StyleId::DarkTrap;
    int rootPc = 0;           // 0 = C ... 11 = B
    int scaleOverride = -1;   // -1 = use the style's scale, else ScaleId
    int bars = 4;             // 1, 2, 4 or 8
    double density = 0.5;     // 0..1 how many notes
    double darkness = 0.5;    // 0..1 tension: b2 colour, chromatic approach notes, dissonance kept
    double swing = 0.0;       // 0..1 (1 = full triplet swing on 16ths)
    double humanize = 0.3;    // 0..1 velocity + timing variation
    int octaveShift = 0;      // melody octave offset
    bool melody = true;
    bool chords = true;
    bool bass = true;
    uint32_t seed = 1;
};

// The scale actually used after the darkness tweak is applied.
ScaleSteps effectiveScale (const Settings& settings);

// Deterministic: the same settings always give the same pattern.
Pattern generate (const Settings& settings);

} // namespace basement
