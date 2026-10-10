#pragma once

#include "Theory.h"

#include <array>
#include <string>
#include <vector>

namespace basement
{

enum class StyleId
{
    DarkTrap,
    MemphisPhonk,
    BoomBap,
    DetroitFlint,
    Plugg,
    Rage,
    Drill,
    Angelic,
    Ethereal,
    YeatBells,
    Count
};

// Everything that makes one sub-genre sound like itself. The generator is the
// same for every style; only these numbers change.
struct StyleDef
{
    std::string name;
    std::string description;
    int defaultTempo;

    // Harmony
    ScaleId scale;
    std::vector<int> melodyDegrees;              // scale degrees (0..6) the melody may land on
    std::vector<std::vector<int>> progressions;  // chord roots as scale degrees
    bool sevenths;
    bool ninths = false;                         // add the 9th on top (needs sevenths)
    bool chordStabs;                             // short rhythmic chord hits instead of pads

    // Rhythm (16th-note grid, onset probability at density 0.5)
    std::array<double, 16> rhythm;
    double tripletChance;                        // per beat: use an 8th-note triplet grid instead
    double legato;                               // note length as fraction of the gap to the next note
    double maxNoteBeats;
    std::array<double, 16> bassRhythm;

    // Phrasing: motif length and how motifs repeat ("AABA'", ...)
    int motifBars;
    std::vector<std::string> forms;

    // Pitch movement
    double repeatProb, stepProb, leapProb, octaveProb;
    double chordToneBias;                        // strong beats snap to chord tones
    double chromaticity;                         // chance of chromatic approach notes (scaled by darkness)
    int melodyCenter;                            // MIDI note the melody hovers around
    int rangeLow, rangeHigh;                     // in scale degrees relative to tonic
    double bassOctaveJumpChance;
};

const StyleDef& getStyle (StyleId id);
int numStyles();

// Accepts the display name or a short slug ("rage", "memphis", "boombap", ...).
StyleId parseStyle (const std::string& name, bool& ok);

} // namespace basement
