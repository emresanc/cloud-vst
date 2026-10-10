#include "Styles.h"

#include <algorithm>
#include <cctype>

namespace basement
{

static std::vector<StyleDef> buildStyles()
{
    std::vector<StyleDef> s;

    // --- Dark Trap: sparse, eerie harmonic-minor bells/pianos -----------------
    {
        StyleDef d;
        d.name = "Dark Trap";
        d.description = "Eerie harmonic-minor bells and pianos, lots of space for the 808.";
        d.defaultTempo = 145;
        d.scale = ScaleId::HarmonicMinor;
        d.melodyDegrees = { 0, 1, 2, 3, 4, 5, 6 };
        d.progressions = { { 0, 5, 3, 4 }, { 0, 0, 5, 4 }, { 0, 3, 5, 4 }, { 0, 1, 0, 4 }, { 0, 5, 0, 4 } };
        d.sevenths = false;
        d.chordStabs = false;
        d.rhythm = { 0.95, 0.05, 0.25, 0.45, 0.35, 0.05, 0.45, 0.15, 0.75, 0.05, 0.25, 0.45, 0.30, 0.10, 0.45, 0.15 };
        d.tripletChance = 0.12;
        d.legato = 0.9;
        d.maxNoteBeats = 1.5;
        d.bassRhythm = { 1.0, 0, 0, 0.3, 0, 0, 0.5, 0, 0.2, 0, 0.5, 0, 0, 0, 0.3, 0 };
        d.motifBars = 2;
        d.forms = { "AABA'", "ABAB'", "AAAB", "AA'AA''" };
        d.repeatProb = 0.18; d.stepProb = 0.52; d.leapProb = 0.25; d.octaveProb = 0.05;
        d.chordToneBias = 0.75;
        d.chromaticity = 0.25;
        d.melodyCenter = 72;
        d.rangeLow = -3; d.rangeHigh = 9;
        d.bassOctaveJumpChance = 0.12;
        s.push_back (d);
    }

    // --- Memphis / Phonk: phrygian, hypnotic 1-bar loops ------------------------
    {
        StyleDef d;
        d.name = "Memphis Phonk";
        d.description = "Hypnotic one-bar phrygian loops, cowbell-friendly, low and dusty.";
        d.defaultTempo = 130;
        d.scale = ScaleId::Phrygian;
        d.melodyDegrees = { 0, 1, 2, 3, 4, 5, 6 };
        d.progressions = { { 0, 1, 0, 1 }, { 0, 0, 1, 0 }, { 0, 1, 0, 6 }, { 0, 5, 1, 0 } };
        d.sevenths = false;
        d.chordStabs = false;
        d.rhythm = { 0.95, 0.30, 0.65, 0.30, 0.80, 0.30, 0.65, 0.35, 0.90, 0.30, 0.65, 0.30, 0.75, 0.35, 0.65, 0.40 };
        d.tripletChance = 0.05;
        d.legato = 0.6;
        d.maxNoteBeats = 1.0;
        d.bassRhythm = { 1.0, 0, 0, 0.5, 0, 0, 0.6, 0, 0.7, 0, 0, 0.4, 0, 0, 0.5, 0 };
        d.motifBars = 1;
        d.forms = { "AAAA'", "AAAB", "AA'AA'" };
        d.repeatProb = 0.30; d.stepProb = 0.50; d.leapProb = 0.18; d.octaveProb = 0.02;
        d.chordToneBias = 0.65;
        d.chromaticity = 0.15;
        d.melodyCenter = 64;
        d.rangeLow = -2; d.rangeHigh = 7;
        d.bassOctaveJumpChance = 0.05;
        s.push_back (d);
    }

    // --- Boom Bap / Griselda: dusty dorian 7ths, swung -------------------------
    {
        StyleDef d;
        d.name = "Boom Bap";
        d.description = "Dusty dorian 7th chords and sample-chop phrases. Turn swing up.";
        d.defaultTempo = 88;
        d.scale = ScaleId::Dorian;
        d.melodyDegrees = { 0, 1, 2, 3, 4, 5, 6 };
        d.progressions = { { 0, 3, 0, 3 }, { 1, 4, 0, 0 }, { 0, 6, 5, 4 }, { 0, 2, 3, 0 }, { 0, 0, 3, 4 } };
        d.sevenths = true;
        d.chordStabs = false;
        d.rhythm = { 0.90, 0.0, 0.30, 0.40, 0.50, 0.0, 0.40, 0.20, 0.70, 0.0, 0.30, 0.50, 0.40, 0.10, 0.50, 0.10 };
        d.tripletChance = 0.05;
        d.legato = 0.8;
        d.maxNoteBeats = 2.0;
        d.bassRhythm = { 1.0, 0, 0, 0, 0, 0, 0.3, 0.4, 0, 0, 0.6, 0, 0, 0, 0, 0.2 };
        d.motifBars = 2;
        d.forms = { "AABA'", "ABAB'", "AA'AA''" };
        d.repeatProb = 0.12; d.stepProb = 0.55; d.leapProb = 0.30; d.octaveProb = 0.03;
        d.chordToneBias = 0.7;
        d.chromaticity = 0.3;
        d.melodyCenter = 69;
        d.rangeLow = -3; d.rangeHigh = 9;
        d.bassOctaveJumpChance = 0.0;
        s.push_back (d);
    }

    // --- Detroit / Flint: fast, tense, staccato piano runs ---------------------
    {
        StyleDef d;
        d.name = "Detroit / Flint";
        d.description = "Fast, tense staccato piano runs and stabs. Busy, nervous energy.";
        d.defaultTempo = 100;
        d.scale = ScaleId::NaturalMinor;
        d.melodyDegrees = { 0, 1, 2, 3, 4, 5, 6 };
        d.progressions = { { 0, 0, 5, 5 }, { 0, 6, 5, 6 }, { 0, 3, 4, 0 }, { 0, 5, 2, 6 } };
        d.sevenths = false;
        d.chordStabs = true;
        d.rhythm = { 0.95, 0.65, 0.65, 0.70, 0.85, 0.60, 0.65, 0.70, 0.95, 0.65, 0.65, 0.70, 0.85, 0.60, 0.65, 0.70 };
        d.tripletChance = 0.08;
        d.legato = 0.45;
        d.maxNoteBeats = 0.5;
        d.bassRhythm = { 1.0, 0, 0, 0.5, 0, 0, 0.5, 0, 0.3, 0, 0.5, 0, 0, 0.3, 0, 0 };
        d.motifBars = 1;
        d.forms = { "AAAB", "AA'AA''", "ABAB'" };
        d.repeatProb = 0.20; d.stepProb = 0.55; d.leapProb = 0.20; d.octaveProb = 0.05;
        d.chordToneBias = 0.6;
        d.chromaticity = 0.4;
        d.melodyCenter = 72;
        d.rangeLow = -2; d.rangeHigh = 9;
        d.bassOctaveJumpChance = 0.08;
        s.push_back (d);
    }

    // --- Plugg / Pluggnb: dreamy major 7ths, sparse bells ----------------------
    {
        StyleDef d;
        d.name = "Plugg";
        d.description = "Classic plugg: dreamy major-7th chords, soft sparse pentatonic leads.";
        d.defaultTempo = 150;
        d.scale = ScaleId::Major;
        d.melodyDegrees = { 0, 1, 2, 4, 5 }; // major pentatonic
        d.progressions = { { 3, 4, 2, 5 }, { 0, 5, 3, 4 }, { 5, 3, 0, 4 }, { 3, 2, 1, 0 }, { 3, 4, 5, 5 } };
        d.sevenths = true;
        d.chordStabs = false;
        d.rhythm = { 0.85, 0.0, 0.20, 0.30, 0.30, 0.0, 0.45, 0.0, 0.65, 0.0, 0.20, 0.30, 0.20, 0.0, 0.35, 0.0 };
        d.tripletChance = 0.10;
        d.legato = 0.95;
        d.maxNoteBeats = 2.0;
        d.bassRhythm = { 1.0, 0, 0, 0, 0, 0, 0.4, 0, 0, 0, 0.5, 0, 0, 0, 0, 0 };
        d.motifBars = 2;
        d.forms = { "AABA'", "ABAB'", "AA'AA''" };
        d.repeatProb = 0.10; d.stepProb = 0.50; d.leapProb = 0.32; d.octaveProb = 0.08;
        d.chordToneBias = 0.7;
        d.chromaticity = 0.05;
        d.melodyCenter = 76;
        d.rangeLow = -2; d.rangeHigh = 10;
        d.bassOctaveJumpChance = 0.1;
        s.push_back (d);
    }

    // --- Rage: minor-pentatonic synth leads, octave jumps, 16th arps -----------
    {
        StyleDef d;
        d.name = "Rage";
        d.description = "Bright-dark synth leads: minor pentatonic, octave jumps, 16th arps.";
        d.defaultTempo = 160;
        d.scale = ScaleId::NaturalMinor;
        d.melodyDegrees = { 0, 2, 3, 4, 6 }; // minor pentatonic
        d.progressions = { { 0, 5, 2, 6 }, { 0, 6, 5, 6 }, { 5, 6, 0, 0 }, { 0, 3, 5, 6 } };
        d.sevenths = false;
        d.chordStabs = false;
        d.rhythm = { 0.95, 0.35, 0.65, 0.50, 0.85, 0.35, 0.65, 0.50, 0.95, 0.35, 0.65, 0.50, 0.85, 0.40, 0.70, 0.55 };
        d.tripletChance = 0.03;
        d.legato = 0.75;
        d.maxNoteBeats = 1.0;
        d.bassRhythm = { 1.0, 0, 0, 0.6, 0, 0, 0.6, 0, 0, 0, 0.6, 0, 0.3, 0, 0.3, 0 };
        d.motifBars = 1;
        d.forms = { "AAAB", "AA'AA'", "ABAB'" };
        d.repeatProb = 0.15; d.stepProb = 0.40; d.leapProb = 0.25; d.octaveProb = 0.20;
        d.chordToneBias = 0.6;
        d.chromaticity = 0.0;
        d.melodyCenter = 74;
        d.rangeLow = -2; d.rangeHigh = 9;
        d.bassOctaveJumpChance = 0.15;
        s.push_back (d);
    }

    // --- Drill / Horror: triplet counter-melodies, harmonic minor --------------
    {
        StyleDef d;
        d.name = "Drill";
        d.description = "Sliding triplet counter-melodies over horror-movie harmonic minor.";
        d.defaultTempo = 142;
        d.scale = ScaleId::HarmonicMinor;
        d.melodyDegrees = { 0, 1, 2, 3, 4, 5, 6 };
        d.progressions = { { 0, 5, 0, 4 }, { 0, 3, 4, 4 }, { 0, 5, 3, 4 }, { 0, 0, 5, 1 } };
        d.sevenths = false;
        d.chordStabs = false;
        d.rhythm = { 0.90, 0.0, 0.20, 0.60, 0.20, 0.20, 0.60, 0.0, 0.50, 0.0, 0.40, 0.50, 0.20, 0.40, 0.30, 0.20 };
        d.tripletChance = 0.35;
        d.legato = 0.85;
        d.maxNoteBeats = 1.5;
        d.bassRhythm = { 1.0, 0, 0, 0, 0, 0, 0.6, 0, 0, 0, 0.5, 0, 0, 0.4, 0, 0 };
        d.motifBars = 2;
        d.forms = { "AABA'", "ABAB'", "AAAB" };
        d.repeatProb = 0.14; d.stepProb = 0.56; d.leapProb = 0.25; d.octaveProb = 0.05;
        d.chordToneBias = 0.7;
        d.chromaticity = 0.3;
        d.melodyCenter = 72;
        d.rangeLow = -3; d.rangeHigh = 8;
        d.bassOctaveJumpChance = 0.1;
        s.push_back (d);
    }

    // --- Angelic / Jerk: lydian maj9 loops, high hypnotic synths ---------------
    {
        StyleDef d;
        d.name = "Angelic";
        d.description = "Angelic new-wave underground: lydian maj9 loops, high hypnotic 1-bar synths.";
        d.defaultTempo = 158;
        d.scale = ScaleId::Lydian;
        d.melodyDegrees = { 0, 1, 2, 3, 4, 5, 6 };
        d.progressions = { { 0, 1, 0, 1 }, { 0, 0, 1, 1 }, { 5, 1, 0, 0 }, { 0, 4, 5, 1 } };
        d.sevenths = true;
        d.ninths = true;
        d.chordStabs = false;
        d.rhythm = { 0.95, 0.20, 0.60, 0.30, 0.75, 0.20, 0.60, 0.35, 0.90, 0.20, 0.60, 0.30, 0.70, 0.30, 0.65, 0.40 };
        d.tripletChance = 0.05;
        d.legato = 0.85;
        d.maxNoteBeats = 1.0;
        d.bassRhythm = { 1.0, 0, 0, 0.5, 0, 0, 0.5, 0, 0, 0, 0.6, 0, 0.3, 0, 0.3, 0 };
        d.motifBars = 1;
        d.forms = { "AAAA'", "AA'AA'", "AAAB" };
        d.repeatProb = 0.22; d.stepProb = 0.45; d.leapProb = 0.20; d.octaveProb = 0.13;
        d.chordToneBias = 0.6;
        d.chromaticity = 0.0;
        d.melodyCenter = 76;
        d.rangeLow = -2; d.rangeHigh = 8;
        d.bassOctaveJumpChance = 0.15;
        s.push_back (d);
    }

    // --- Ethereal sample: washed-out dorian m9 loops, long notes ---------------
    {
        StyleDef d;
        d.name = "Ethereal Sample";
        d.description = "Washed-out sample-flip feel: dorian m9 loops, long hazy notes. Add tape + reverb.";
        d.defaultTempo = 150;
        d.scale = ScaleId::Dorian;
        d.melodyDegrees = { 0, 1, 2, 3, 4, 5, 6 };
        d.progressions = { { 0, 3, 0, 3 }, { 0, 6, 0, 6 }, { 2, 3, 0, 0 }, { 0, 0, 6, 3 } };
        d.sevenths = true;
        d.ninths = true;
        d.chordStabs = false;
        d.rhythm = { 0.90, 0.0, 0.10, 0.15, 0.40, 0.0, 0.20, 0.05, 0.60, 0.0, 0.15, 0.20, 0.30, 0.0, 0.25, 0.05 };
        d.tripletChance = 0.08;
        d.legato = 1.0;
        d.maxNoteBeats = 3.0;
        d.bassRhythm = { 1.0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.4, 0, 0, 0, 0, 0 };
        d.motifBars = 2;
        d.forms = { "AABA'", "AA'AA''", "ABAB'" };
        d.repeatProb = 0.20; d.stepProb = 0.60; d.leapProb = 0.18; d.octaveProb = 0.02;
        d.chordToneBias = 0.75;
        d.chromaticity = 0.1;
        d.melodyCenter = 70;
        d.rangeLow = -3; d.rangeHigh = 8;
        d.bassOctaveJumpChance = 0.05;
        s.push_back (d);
    }

    // --- Bells: high repetitive 16th bell riffs, octave bounces ----------------
    {
        StyleDef d;
        d.name = "Bells";
        d.description = "High, repetitive 16th bell riffs with octave bounces. Short and percussive.";
        d.defaultTempo = 160;
        d.scale = ScaleId::NaturalMinor;
        d.melodyDegrees = { 0, 2, 3, 4, 6 }; // minor pentatonic
        d.progressions = { { 0, 0, 5, 5 }, { 0, 5, 2, 6 }, { 5, 6, 0, 0 }, { 0, 6, 0, 6 } };
        d.sevenths = false;
        d.chordStabs = false;
        d.rhythm = { 0.95, 0.50, 0.75, 0.50, 0.85, 0.50, 0.75, 0.55, 0.95, 0.50, 0.75, 0.50, 0.85, 0.55, 0.75, 0.60 };
        d.tripletChance = 0.02;
        d.legato = 0.5;
        d.maxNoteBeats = 0.5;
        d.bassRhythm = { 1.0, 0, 0, 0.6, 0, 0, 0.6, 0, 0, 0, 0.6, 0, 0.3, 0, 0.3, 0 };
        d.motifBars = 1;
        d.forms = { "AAAA'", "AAAB", "AA'AA'" };
        d.repeatProb = 0.35; d.stepProb = 0.30; d.leapProb = 0.15; d.octaveProb = 0.20;
        d.chordToneBias = 0.55;
        d.chromaticity = 0.0;
        d.melodyCenter = 80;
        d.rangeLow = -2; d.rangeHigh = 7;
        d.bassOctaveJumpChance = 0.15;
        s.push_back (d);
    }

    // --- Vicious: dark, manic, aggressive phrygian riffs -----------------------
    {
        StyleDef d;
        d.name = "Vicious";
        d.description = "Dark, manic and aggressive: low phrygian riffs on 8ths, busy 808. Distort it.";
        d.defaultTempo = 155;
        d.scale = ScaleId::Phrygian;
        d.melodyDegrees = { 0, 1, 2, 4, 5 }; // 1 b2 b3 5 b6: the evil notes only
        d.progressions = { { 0, 1, 0, 0 }, { 0, 5, 1, 0 }, { 0, 0, 5, 1 }, { 0, 1, 5, 1 } };
        d.sevenths = false;
        d.chordStabs = false;
        d.rhythm = { 0.95, 0.20, 0.80, 0.30, 0.90, 0.20, 0.80, 0.40, 0.95, 0.20, 0.80, 0.30, 0.90, 0.30, 0.80, 0.45 };
        d.tripletChance = 0.04;
        d.legato = 0.65;
        d.maxNoteBeats = 0.75;
        d.bassRhythm = { 1.0, 0, 0, 0.6, 0, 0, 0.7, 0, 0.4, 0, 0.6, 0, 0.3, 0, 0.6, 0.3 };
        d.motifBars = 1;
        d.forms = { "AAAA'", "AAAB", "AA'AA'" };
        d.repeatProb = 0.35; d.stepProb = 0.40; d.leapProb = 0.15; d.octaveProb = 0.10;
        d.chordToneBias = 0.6;
        d.chromaticity = 0.3;
        d.melodyCenter = 62;
        d.rangeLow = -2; d.rangeHigh = 7;
        d.bassOctaveJumpChance = 0.25;
        s.push_back (d);
    }

    // --- Slayer: spine-chilling harmonic-minor piano, sliding triplets ---------
    {
        StyleDef d;
        d.name = "Slayer";
        d.description = "Spine-chilling trap: medieval harmonic-minor piano, eerie m(maj7) chords, sliding triplets.";
        d.defaultTempo = 150;
        d.scale = ScaleId::HarmonicMinor;
        d.melodyDegrees = { 0, 1, 2, 3, 4, 5, 6 };
        d.progressions = { { 0, 5, 4, 0 }, { 0, 3, 4, 4 }, { 0, 5, 1, 4 }, { 0, 0, 5, 4 } };
        d.sevenths = true;
        d.chordStabs = false;
        d.rhythm = { 0.90, 0.05, 0.30, 0.50, 0.40, 0.10, 0.55, 0.15, 0.75, 0.05, 0.30, 0.50, 0.35, 0.15, 0.50, 0.20 };
        d.tripletChance = 0.18;
        d.legato = 0.9;
        d.maxNoteBeats = 1.5;
        d.bassRhythm = { 1.0, 0, 0, 0.3, 0, 0, 0.5, 0, 0.2, 0, 0.5, 0, 0, 0, 0.3, 0 };
        d.motifBars = 2;
        d.forms = { "AABA'", "ABAB'", "AA'AA''" };
        d.repeatProb = 0.12; d.stepProb = 0.58; d.leapProb = 0.25; d.octaveProb = 0.05;
        d.chordToneBias = 0.7;
        d.chromaticity = 0.35;
        d.melodyCenter = 70;
        d.rangeLow = -3; d.rangeHigh = 9;
        d.bassOctaveJumpChance = 0.1;
        s.push_back (d);
    }

    // --- Nightmare: unearthly, slow huge synth lines over dark 7ths ------------
    {
        StyleDef d;
        d.name = "Nightmare";
        d.description = "Nightmarish, unearthly: slow wide synth lines and octave drops over dark 7ths.";
        d.defaultTempo = 150;
        d.scale = ScaleId::NaturalMinor;
        d.melodyDegrees = { 0, 1, 2, 3, 4, 5, 6 };
        d.progressions = { { 0, 5, 0, 5 }, { 0, 3, 5, 4 }, { 5, 3, 0, 0 }, { 0, 6, 5, 5 } };
        d.sevenths = true;
        d.chordStabs = false;
        d.rhythm = { 0.90, 0.0, 0.10, 0.20, 0.35, 0.0, 0.25, 0.05, 0.65, 0.0, 0.15, 0.25, 0.30, 0.0, 0.30, 0.05 };
        d.tripletChance = 0.06;
        d.legato = 1.0;
        d.maxNoteBeats = 2.5;
        d.bassRhythm = { 1.0, 0, 0, 0, 0, 0, 0.5, 0, 0, 0, 0.5, 0, 0, 0, 0.2, 0 };
        d.motifBars = 2;
        d.forms = { "AABA'", "AA'AA''", "AAAB" };
        d.repeatProb = 0.15; d.stepProb = 0.45; d.leapProb = 0.25; d.octaveProb = 0.15;
        d.chordToneBias = 0.75;
        d.chromaticity = 0.2;
        d.melodyCenter = 72;
        d.rangeLow = -4; d.rangeHigh = 8;
        d.bassOctaveJumpChance = 0.05;
        s.push_back (d);
    }

    return s;
}

static const std::vector<StyleDef>& styles()
{
    static const std::vector<StyleDef> table = buildStyles();
    return table;
}

const StyleDef& getStyle (StyleId id) { return styles()[(size_t) id]; }
int numStyles() { return (int) styles().size(); }

static std::string slug (const std::string& s)
{
    std::string out;
    for (char c : s)
        if (std::isalnum ((unsigned char) c))
            out += (char) std::tolower ((unsigned char) c);
    return out;
}

StyleId parseStyle (const std::string& name, bool& ok)
{
    const auto key = slug (name);
    ok = true;

    for (int i = 0; i < numStyles(); ++i)
        if (slug (getStyle ((StyleId) i).name) == key)
            return (StyleId) i;

    static const std::pair<const char*, StyleId> aliases[] = {
        { "trap", StyleId::DarkTrap },     { "darktrap", StyleId::DarkTrap },
        { "memphis", StyleId::MemphisPhonk }, { "phonk", StyleId::MemphisPhonk },
        { "boombap", StyleId::BoomBap },   { "griselda", StyleId::BoomBap },
        { "detroit", StyleId::DetroitFlint }, { "flint", StyleId::DetroitFlint },
        { "plugg", StyleId::Plugg },       { "pluggnb", StyleId::Plugg },
        { "rage", StyleId::Rage },         { "drill", StyleId::Drill },
        { "angelic", StyleId::Angelic },   { "jerk", StyleId::Angelic },
        { "ethereal", StyleId::Ethereal }, { "sample", StyleId::Ethereal },
        { "bells", StyleId::YeatBells },   { "bell", StyleId::YeatBells },
        { "vicious", StyleId::Vicious },   { "slayer", StyleId::Slayer },
        { "nightmare", StyleId::Nightmare },
    };

    for (auto& a : aliases)
        if (key == a.first)
            return a.second;

    ok = false;
    return StyleId::DarkTrap;
}

} // namespace basement
