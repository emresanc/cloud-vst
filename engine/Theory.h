#pragma once

#include <array>
#include <string>
#include <vector>

namespace basement
{

using ScaleSteps = std::array<int, 7>;

enum class ScaleId
{
    NaturalMinor,
    HarmonicMinor,
    Phrygian,
    Dorian,
    PhrygianDominant,
    Major,
    Count
};

struct ScaleDef
{
    const char* name;
    ScaleSteps steps;
};

const ScaleDef& getScale (ScaleId id);
int numScales();

inline int floorDiv (int a, int b) { return (a >= 0) ? a / b : -((-a + b - 1) / b); }
inline int wrap (int a, int b) { return a - floorDiv (a, b) * b; }

// Semitone offset of a (possibly negative or > 6) scale degree from the tonic.
inline int degreeToSemitone (const ScaleSteps& steps, int degree)
{
    const int oct = floorDiv (degree, 7);
    return steps[(size_t) (degree - oct * 7)] + 12 * oct;
}

const std::vector<std::string>& noteNames();
std::string noteName (int pitchClass);
std::string midiNoteName (int midiNote); // e.g. "C#4"

// Name a chord from its root pitch class and the semitone offsets of its tones
// above the root (e.g. {0, 3, 7, 10} -> "m7").
std::string chordName (int rootPc, const std::vector<int>& intervalsFromRoot);

// Parses "C", "C#", "Db", "f#" ... returns -1 on failure.
int parseNoteName (const std::string& s);

} // namespace basement
