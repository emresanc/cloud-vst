// Dependency-free tests for the melody engine. Run via `ctest` or directly.

#include "Generator.h"
#include "MidiWriter.h"

#include <cstdio>
#include <set>
#include <string>

using namespace basement;

static int failures = 0;

#define CHECK(cond)                                                              \
    do                                                                           \
    {                                                                            \
        if (! (cond))                                                            \
        {                                                                        \
            std::printf ("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);         \
            ++failures;                                                          \
        }                                                                        \
    } while (0)

static bool samePattern (const Pattern& a, const Pattern& b)
{
    if (a.notes.size() != b.notes.size())
        return false;
    for (size_t i = 0; i < a.notes.size(); ++i)
    {
        const auto &x = a.notes[i], &y = b.notes[i];
        if (x.pitch != y.pitch || x.start != y.start || x.length != y.length || x.velocity != y.velocity || x.channel != y.channel)
            return false;
    }
    return true;
}

static void testTheory()
{
    CHECK (parseNoteName ("C") == 0);
    CHECK (parseNoteName ("f#") == 6);
    CHECK (parseNoteName ("Bb") == 10);
    CHECK (parseNoteName ("H") == -1);
    CHECK (chordName (0, { 0, 3, 7 }) == "Cm");
    CHECK (chordName (8, { 0, 4, 7 }) == "G#");
    CHECK (chordName (5, { 0, 4, 7, 11 }) == "Fmaj7");
    CHECK (chordName (2, { 0, 3, 6 }) == "Ddim");
    CHECK (chordName (9, { 0, 3, 7, 10 }) == "Am7");
    CHECK (chordName (0, { 0, 4, 7, 11, 14 }) == "Cmaj9");
    CHECK (chordName (2, { 0, 3, 7, 10, 14 }) == "Dm9");
    CHECK (chordName (4, { 0, 3, 7, 10, 13 }) == "Em7(b9)");
    CHECK (degreeToSemitone (getScale (ScaleId::NaturalMinor).steps, -1) == -2);
    CHECK (degreeToSemitone (getScale (ScaleId::NaturalMinor).steps, 9) == 15);
    CHECK (midiNoteName (60) == "C4");

    bool ok;
    CHECK (parseStyle ("memphis", ok) == StyleId::MemphisPhonk && ok);
    CHECK (parseStyle ("Boom Bap", ok) == StyleId::BoomBap && ok);
    parseStyle ("polka", ok);
    CHECK (! ok);
}

static void testEveryStyle()
{
    for (int st = 0; st < numStyles(); ++st)
        for (int bars : { 1, 2, 4, 8 })
            for (uint32_t seed = 1; seed <= 40; ++seed)
            {
                Settings s;
                s.style = (StyleId) st;
                s.bars = bars;
                s.seed = seed;
                s.rootPc = (int) (seed % 12);
                s.density = (seed % 5) / 4.0;
                s.darkness = 0.0; // no chromatic notes -> every melody note is in the scale
                s.swing = (seed % 3) / 2.0;

                const auto p = generate (s);
                const auto& style = getStyle (s.style);
                const auto steps = effectiveScale (s);

                std::set<int> scalePcs;
                for (auto st2 : steps)
                    scalePcs.insert ((s.rootPc + st2) % 12);

                int melody = 0, chords = 0, bass = 0;
                for (auto& n : p.notes)
                {
                    CHECK (n.pitch >= 0 && n.pitch <= 127);
                    CHECK (n.velocity >= 1 && n.velocity <= 127);
                    CHECK (n.start >= 0 && n.start < p.lengthBeats());
                    CHECK (n.length > 0 && n.start + n.length <= p.lengthBeats() + 1e-9);
                    CHECK (scalePcs.count (n.pitch % 12) == 1);

                    if (n.channel == MelodyChannel)
                    {
                        ++melody;
                        const int pc = ((n.pitch - s.rootPc) % 12 + 12) % 12;
                        bool inMelodyScale = false;
                        for (auto d : style.melodyDegrees)
                            inMelodyScale = inMelodyScale || steps[(size_t) d] == pc;
                        CHECK (inMelodyScale);
                        CHECK (n.pitch >= 40 && n.pitch <= 100);
                    }
                    else if (n.channel == ChordChannel)
                        ++chords;
                    else if (n.channel == BassChannel)
                    {
                        ++bass;
                        CHECK (n.pitch >= 28 && n.pitch <= 52);
                    }
                }

                CHECK (melody >= 2);
                CHECK (chords >= 3);
                CHECK (bass >= 1);
                CHECK (! p.chords.empty());
                CHECK (p.lengthBeats() == bars * 4);
            }
}

static void testDeterminismAndVariety()
{
    Settings s;
    s.style = StyleId::Rage;
    s.seed = 1234;
    CHECK (samePattern (generate (s), generate (s)));

    std::set<std::string> distinct;
    for (uint32_t seed = 0; seed < 30; ++seed)
    {
        s.seed = seed;
        std::string sig;
        for (auto& n : generate (s).notes)
            if (n.channel == MelodyChannel)
                sig += std::to_string (n.pitch) + "@" + std::to_string ((int) (n.start * 12)) + ",";
        distinct.insert (sig);
    }
    CHECK (distinct.size() >= 28);

    // Layer toggles must not change the other layers.
    s.seed = 77;
    const auto full = generate (s);
    s.chords = false;
    s.bass = false;
    const auto melodyOnly = generate (s);
    int fullMelody = 0;
    for (auto& n : full.notes)
        fullMelody += n.channel == MelodyChannel;
    CHECK ((int) melodyOnly.notes.size() == fullMelody);
}

static void testDensity()
{
    for (int st = 0; st < numStyles(); ++st)
    {
        size_t sparse = 0, busy = 0;
        for (uint32_t seed = 1; seed <= 20; ++seed)
        {
            Settings s;
            s.style = (StyleId) st;
            s.seed = seed;
            s.chords = s.bass = false;
            s.density = 0.0;
            sparse += generate (s).notes.size();
            s.density = 1.0;
            busy += generate (s).notes.size();
        }
        CHECK (busy > sparse);
    }
}

static void testMidiFile()
{
    Settings s;
    s.bars = 2;
    const auto p = generate (s);
    const auto bytes = writeMidiFile (p, 140.0);

    CHECK (bytes.size() > 30);
    CHECK (std::string (bytes.begin(), bytes.begin() + 4) == "MThd");
    CHECK (bytes[9] == 1);   // format 1
    CHECK (bytes[11] == 4);  // tempo + 3 layer tracks
    CHECK (bytes[12] == 0x03 && bytes[13] == 0xC0); // 960 ppq

    const auto melodyOnly = writeMidiFile (p, 140.0, LayerMelody);
    CHECK (melodyOnly[11] == 2);

    // Walk the chunks and check their lengths add up.
    size_t pos = 14, chunks = 0;
    while (pos + 8 <= melodyOnly.size())
    {
        CHECK (std::string (melodyOnly.begin() + (long) pos, melodyOnly.begin() + (long) pos + 4) == "MTrk");
        const uint32_t len = (uint32_t) melodyOnly[pos + 4] << 24 | (uint32_t) melodyOnly[pos + 5] << 16
                             | (uint32_t) melodyOnly[pos + 6] << 8 | melodyOnly[pos + 7];
        pos += 8 + len;
        ++chunks;
    }
    CHECK (pos == melodyOnly.size());
    CHECK (chunks == 2);
}

int main()
{
    testTheory();
    testEveryStyle();
    testDeterminismAndVariety();
    testDensity();
    testMidiFile();

    if (failures == 0)
        std::printf ("all engine tests passed\n");
    return failures == 0 ? 0 : 1;
}
