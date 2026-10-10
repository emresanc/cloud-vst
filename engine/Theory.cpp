#include "Theory.h"

#include <algorithm>
#include <cctype>

namespace basement
{

static const ScaleDef scales[] = {
    { "Natural Minor", { 0, 2, 3, 5, 7, 8, 10 } },
    { "Harmonic Minor", { 0, 2, 3, 5, 7, 8, 11 } },
    { "Phrygian", { 0, 1, 3, 5, 7, 8, 10 } },
    { "Dorian", { 0, 2, 3, 5, 7, 9, 10 } },
    { "Phrygian Dominant", { 0, 1, 4, 5, 7, 8, 10 } },
    { "Major", { 0, 2, 4, 5, 7, 9, 11 } },
    { "Lydian", { 0, 2, 4, 6, 7, 9, 11 } },
};

static_assert (sizeof (scales) / sizeof (scales[0]) == (size_t) ScaleId::Count, "scale table mismatch");

const ScaleDef& getScale (ScaleId id) { return scales[(size_t) id]; }
int numScales() { return (int) ScaleId::Count; }

const std::vector<std::string>& noteNames()
{
    static const std::vector<std::string> names { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return names;
}

std::string noteName (int pitchClass) { return noteNames()[(size_t) wrap (pitchClass, 12)]; }

std::string midiNoteName (int midiNote)
{
    return noteName (midiNote) + std::to_string (floorDiv (midiNote, 12) - 1);
}

std::string chordName (int rootPc, const std::vector<int>& intervals)
{
    auto has = [&] (int semis) { return std::find (intervals.begin(), intervals.end(), semis) != intervals.end(); };

    const bool minor3 = has (3), major3 = has (4);
    const bool dim5 = has (6), aug5 = has (8) && ! has (7);
    const bool min7 = has (10), maj7 = has (11), dim7 = has (9) && dim5 && minor3;
    const bool hasSeventh = intervals.size() >= 4;

    const bool hasNinth = intervals.size() >= 5;
    std::string q;

    if (minor3 && dim5)
    {
        if (! hasSeventh)
            q = "dim";
        else if (min7)
            q = "m7b5";
        else if (dim7)
            q = "dim7";
        else
            q = "dim(maj7)";
    }
    else if (major3 && aug5)
    {
        q = hasSeventh && maj7 ? "maj7#5" : "aug";
    }
    else if (minor3)
    {
        q = ! hasSeventh ? "m" : (maj7 ? "m(maj7)" : "m7");
    }
    else if (major3)
    {
        q = ! hasSeventh ? "" : (maj7 ? "maj7" : "7");
    }
    else
    {
        q = "5";
    }

    if (hasNinth)
    {
        if (has (13))        q += "(b9)";
        else if (q == "maj7") q = "maj9";
        else if (q == "m7")  q = "m9";
        else if (q == "7")   q = "9";
        else                 q += "(9)";
    }

    return noteName (rootPc) + q;
}

int parseNoteName (const std::string& raw)
{
    if (raw.empty())
        return -1;

    static const int base[] = { 9, 11, 0, 2, 4, 5, 7 }; // A B C D E F G
    const char letter = (char) std::toupper ((unsigned char) raw[0]);
    if (letter < 'A' || letter > 'G')
        return -1;

    int pc = base[letter - 'A'];
    for (size_t i = 1; i < raw.size(); ++i)
    {
        const char c = raw[i];
        if (c == '#' || c == 's' || c == 'S')
            ++pc;
        else if (c == 'b' || c == 'B')
            --pc;
        else
            return -1;
    }
    return wrap (pc, 12);
}

} // namespace basement
