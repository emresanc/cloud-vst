#include "Generator.h"
#include "Random.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <map>

namespace basement
{

std::string Pattern::chordSummary() const
{
    std::string s;
    for (size_t i = 0; i < chords.size(); ++i)
        s += (i ? " | " : "") + chords[i].name;
    return s;
}

ScaleSteps effectiveScale (const Settings& settings)
{
    const auto& style = getStyle (settings.style);
    const auto id = settings.scaleOverride >= 0 && settings.scaleOverride < numScales()
                        ? (ScaleId) settings.scaleOverride
                        : style.scale;

    auto steps = getScale (id).steps;

    // Very dark settings flatten the 2nd of minor scales (the phrygian b2):
    // the single most "evil" interval in underground beats.
    if (settings.darkness > 0.66 && steps[2] == 3 && steps[1] == 2)
        steps[1] = 1;

    return steps;
}

namespace
{

struct MNote
{
    double start;
    double length;
    int degree;
    bool strong;
};

using Motif = std::vector<MNote>;

struct FormSlot
{
    char motif;
    int variation;
};

std::vector<FormSlot> parseForm (const std::string& form)
{
    std::vector<FormSlot> slots;
    for (char c : form)
    {
        if (c == '\'' && ! slots.empty())
            ++slots.back().variation;
        else if (c != '\'')
            slots.push_back ({ c, 0 });
    }
    return slots;
}

// Expand a 4-slot form to the number of motif slots the pattern needs.
std::vector<FormSlot> formForSlots (const std::string& form, int numSlots)
{
    const auto base = parseForm (form);
    std::vector<FormSlot> out;

    if (numSlots <= 1)
        return { { 'A', 0 } };

    if (numSlots == 2)
        return { { 'A', 0 }, { 'A', 1 } };

    for (int i = 0; i < numSlots; ++i)
    {
        auto s = base[(size_t) (i % (int) base.size())];
        // Second half of a long pattern: push the ending further away from the first.
        if (i >= (int) base.size() && i == numSlots - 1)
            s.variation += 1;
        out.push_back (s);
    }
    return out;
}

class Builder
{
public:
    explicit Builder (const Settings& s)
        : set (s),
          style (getStyle (s.style)),
          steps (effectiveScale (s)),
          harmonyRng (s.seed ^ 0x51ED2701u),
          melodyRng ((uint64_t) s.seed * 31 + 0xB5u),
          bassRng ((uint64_t) s.seed * 131 + 0x808u),
          feelRng ((uint64_t) s.seed * 7919 + 0x4u)
    {
        pattern.bars = std::clamp (s.bars, 1, 16);
        motifBars = std::min (style.motifBars, pattern.bars);
        density = std::clamp (s.density, 0.0, 1.0);
        darkness = std::clamp (s.darkness, 0.0, 1.0);

        const int center = style.melodyCenter + 12 * s.octaveShift;
        melodyBase = center - wrap (center - s.rootPc, 12);
        if (center - melodyBase > 6)
            melodyBase += 12;
    }

    Pattern build()
    {
        buildChords();

        if (set.melody)
            buildMelody();
        if (set.chords)
            emitChords();
        if (set.bass)
            buildBass();

        finalise();
        return pattern;
    }

private:
    const Settings& set;
    const StyleDef& style;
    ScaleSteps steps;
    Rng harmonyRng, melodyRng, bassRng, feelRng;
    Pattern pattern;
    int motifBars = 1;
    double density = 0.5, darkness = 0.5;
    int melodyBase = 72;

    //==========================================================================
    // Harmony

    std::vector<int> chordDegrees (int root) const
    {
        std::vector<int> d { root, root + 2, root + 4 };
        if (style.sevenths)
            d.push_back (root + 6);
        if (style.sevenths && style.ninths)
            d.push_back (root + 8);
        return d;
    }

    static std::vector<int> voice (const std::vector<int>& semis, const std::vector<int>* previous)
    {
        std::vector<int> best;
        double bestScore = 1e9;
        const int n = (int) semis.size();

        for (int inv = 0; inv < n; ++inv)
        {
            std::vector<int> v;
            for (int i = 0; i < n; ++i)
            {
                int p = semis[(size_t) ((inv + i) % n)];
                while (! v.empty() && p <= v.back())
                    p += 12;
                v.push_back (p);
            }

            for (int base = 48; base <= 72; base += 12)
            {
                std::vector<int> cand = v;
                const int shift = base - 12 * floorDiv (cand[0], 12);
                for (auto& p : cand)
                    p += shift;

                if (cand.front() < 50 || cand.front() > 62)
                    continue;

                double score = 0;
                if (previous && previous->size() == cand.size())
                {
                    for (size_t i = 0; i < cand.size(); ++i)
                        score += std::abs (cand[i] - (*previous)[i]);
                }
                else
                {
                    double mean = 0;
                    for (auto p : cand)
                        mean += p;
                    score = std::abs (mean / (double) cand.size() - 60.0);
                }

                if (score < bestScore)
                {
                    bestScore = score;
                    best = cand;
                }
            }
        }
        return best;
    }

    void buildChords()
    {
        const auto& prog = harmonyRng.pick (style.progressions);
        const double chordLen = pattern.bars >= 4 ? 4.0 : 2.0;
        const int count = (int) std::lround (pattern.lengthBeats() / chordLen);

        std::vector<int> previous;
        for (int i = 0; i < count; ++i)
        {
            ChordSlot c;
            c.start = i * chordLen;
            c.length = chordLen;
            c.degree = prog[(size_t) (i % (int) prog.size())];

            std::vector<int> semis, intervals;
            for (auto d : chordDegrees (c.degree))
                semis.push_back (set.rootPc + degreeToSemitone (steps, d));
            for (auto p : semis)
                intervals.push_back (p - semis[0]);

            c.name = chordName (semis[0], intervals);
            c.pitches = voice (semis, previous.empty() ? nullptr : &previous);
            previous = c.pitches;
            pattern.chords.push_back (c);
        }
    }

    const ChordSlot& chordAt (double beat) const
    {
        for (auto& c : pattern.chords)
            if (beat >= c.start - 1e-9 && beat < c.start + c.length - 1e-9)
                return c;
        return pattern.chords.back();
    }

    bool allowed (int degree) const
    {
        const int d = wrap (degree, 7);
        return std::find (style.melodyDegrees.begin(), style.melodyDegrees.end(), d) != style.melodyDegrees.end();
    }

    bool isChordTone (int degree, const ChordSlot& c) const
    {
        const int rel = wrap (degree - c.degree, 7);
        return rel == 0 || rel == 2 || rel == 4 || (style.sevenths && rel == 6) || (style.ninths && rel == 1);
    }

    bool inRange (int d) const { return d >= style.rangeLow && d <= style.rangeHigh; }

    template <typename Pred>
    int nearest (int degree, int preferDir, Pred pred) const
    {
        const int dir = preferDir >= 0 ? 1 : -1;
        for (int k = 0; k <= 7; ++k)
        {
            for (int cand : { degree + k * dir, degree - k * dir })
                if (inRange (cand) && pred (cand))
                    return cand;
        }
        return std::clamp (degree, style.rangeLow, style.rangeHigh);
    }

    int nearestAllowed (int degree, int dir) const
    {
        return nearest (degree, dir, [this] (int d) { return allowed (d); });
    }

    int nearestChordTone (int degree, int dir, const ChordSlot& c) const
    {
        const int strict = nearest (degree, dir, [&] (int d) { return allowed (d) && isChordTone (d, c); });
        if (allowed (strict) && isChordTone (strict, c) && std::abs (strict - degree) <= 3)
            return strict;
        return nearest (degree, dir, [&] (int d) { return isChordTone (d, c); });
    }

    int pitchOf (int degree) const { return melodyBase + degreeToSemitone (steps, degree); }

    //==========================================================================
    // Melody

    std::vector<double> onsets (double from, double to)
    {
        // density 0 -> 0.35x, 0.5 -> 1x, 1 -> 1.65x the style's base probabilities
        const double scale = 0.35 + 1.3 * density;
        std::vector<double> on;

        for (int beat = (int) from; beat < (int) std::ceil (to); ++beat)
        {
            const int beatInBar = beat % 4;
            if (melodyRng.chance (style.tripletChance))
            {
                const double w[3] = { style.rhythm[(size_t) beatInBar * 4], 0.45, 0.55 };
                for (int t = 0; t < 3; ++t)
                    if (melodyRng.uniform() < std::min (0.97, w[t] * scale))
                        on.push_back (beat + t / 3.0);
            }
            else
            {
                for (int s = 0; s < 4; ++s)
                    if (melodyRng.uniform() < std::min (0.97, style.rhythm[(size_t) (beatInBar * 4 + s)] * scale))
                        on.push_back (beat + s * 0.25);
            }
        }

        if (on.empty())
            on.push_back (from);
        return on;
    }

    void assignPitches (Motif& m, size_t firstIndex, double slotStart, int& current)
    {
        const double mid = (style.rangeLow + style.rangeHigh) * 0.5;
        const double total = style.repeatProb + style.stepProb + style.leapProb + style.octaveProb;

        for (size_t i = firstIndex; i < m.size(); ++i)
        {
            auto& n = m[i];
            const auto& chord = chordAt (slotStart + n.start);
            n.strong = std::fabs (std::fmod (n.start, 2.0)) < 1e-6;

            const double r = melodyRng.uniform() * total;
            int move;
            if (r < style.repeatProb)
                move = 0;
            else if (r < style.repeatProb + style.stepProb)
                move = 1;
            else if (r < style.repeatProb + style.stepProb + style.leapProb)
                move = melodyRng.range (2, 4);
            else
                move = 7;

            int dir = melodyRng.chance (0.5) ? 1 : -1;
            if (current > mid + 2 && melodyRng.chance (0.7))
                dir = -1;
            else if (current < mid - 2 && melodyRng.chance (0.7))
                dir = 1;

            int target = current + dir * move;
            if (target > style.rangeHigh)
                target = style.rangeHigh - (target - style.rangeHigh);
            if (target < style.rangeLow)
                target = style.rangeLow + (style.rangeLow - target);

            target = nearestAllowed (target, dir);

            if ((n.strong || i == 0) && melodyRng.chance (style.chordToneBias))
                target = nearestChordTone (target, dir, chord);

            n.degree = target;
            current = target;
        }
    }

    void assignLengths (Motif& m, double motifLen) const
    {
        std::sort (m.begin(), m.end(), [] (const MNote& a, const MNote& b) { return a.start < b.start; });
        for (size_t i = 0; i < m.size(); ++i)
        {
            const double next = i + 1 < m.size() ? m[i + 1].start : motifLen;
            m[i].length = std::max (0.08, std::min (style.maxNoteBeats, (next - m[i].start) * style.legato));
        }
    }

    Motif newMotif (double slotStart, double len)
    {
        Motif m;
        for (auto t : onsets (0, len))
            m.push_back ({ t, 0, 0, false });

        // A motif needs at least a call and an answer.
        for (double t : { 0.0, len * 0.5, len * 0.75 })
        {
            if (m.size() >= 2)
                break;
            if (std::none_of (m.begin(), m.end(), [t] (const MNote& n) { return std::fabs (n.start - t) < 1e-6; }))
                m.push_back ({ t, 0, 0, false });
        }

        std::sort (m.begin(), m.end(), [] (const MNote& a, const MNote& b) { return a.start < b.start; });

        int current = nearestChordTone (melodyRng.range (0, 4), 1, chordAt (slotStart));
        assignPitches (m, 0, slotStart, current);
        assignLengths (m, len);
        return m;
    }

    Motif variationOf (const Motif& base, double slotStart, double len)
    {
        const double cut = len * 0.5;
        Motif m;
        for (auto& n : base)
            if (n.start < cut - 1e-9)
                m.push_back (n);

        int current = m.empty() ? 0 : m.back().degree;
        const size_t first = m.size();
        for (auto t : onsets (cut, len))
            m.push_back ({ t, 0, 0, false });

        assignPitches (m, first, slotStart, current);
        assignLengths (m, len);
        return m;
    }

    // When a motif is reused over a different chord, nudge clashing strong-beat
    // notes. Darkness keeps more of the clashes on purpose.
    void fitToHarmony (Motif& m, double slotStart)
    {
        for (auto& n : m)
        {
            if (! n.strong)
                continue;

            const auto& chord = chordAt (slotStart + n.start);
            if (isChordTone (n.degree, chord))
                continue;

            const int pc = wrap (pitchOf (n.degree), 12);
            bool clash = false;
            for (auto p : chord.pitches)
            {
                const int diff = wrap (pc - p, 12);
                clash = clash || diff == 1 || diff == 11;
            }

            if (clash && melodyRng.chance (1.0 - 0.7 * darkness))
                n.degree = nearestChordTone (n.degree, 1, chord);
        }
    }

    void buildMelody()
    {
        const double slotLen = motifBars * pattern.beatsPerBar;
        const int numSlots = std::max (1, pattern.bars / motifBars);
        const auto form = formForSlots (melodyRng.pick (style.forms), numSlots);

        std::map<std::pair<char, int>, Motif> motifs;
        std::vector<MNote> line;
        std::vector<double> lineSlotStart;

        for (int k = 0; k < numSlots; ++k)
        {
            const double slotStart = k * slotLen;
            const auto& f = form[(size_t) k];
            const auto key = std::make_pair (f.motif, f.variation);
            const auto baseKey = std::make_pair (f.motif, 0);

            bool fresh = false;
            if (motifs.find (baseKey) == motifs.end())
            {
                motifs[baseKey] = newMotif (slotStart, slotLen);
                fresh = f.variation == 0;
            }
            if (motifs.find (key) == motifs.end())
                motifs[key] = variationOf (motifs[baseKey], slotStart, slotLen);

            Motif m = motifs[key];
            if (! fresh)
                fitToHarmony (m, slotStart);

            for (auto n : m)
            {
                n.start += slotStart;
                line.push_back (n);
            }
        }

        // Resolve the final note of the loop onto the last chord.
        if (! line.empty())
        {
            auto& last = line.back();
            last.degree = nearestChordTone (last.degree, -1, chordAt (last.start));
        }

        std::vector<int> pitches;
        for (auto& n : line)
            pitches.push_back (pitchOf (n.degree));

        // Chromatic approach notes: a semitone below the next note, on weak beats.
        const double chromaChance = darkness * style.chromaticity;
        for (size_t i = 0; i + 1 < line.size(); ++i)
        {
            const auto& n = line[i];
            const auto& next = line[i + 1];
            if (n.strong || n.length > 0.5 || next.start - (n.start + n.length) > 0.3)
                continue;
            if (melodyRng.chance (chromaChance))
                pitches[i] = pitches[i + 1] - 1;
        }

        for (size_t i = 0; i < line.size(); ++i)
        {
            const auto& n = line[i];
            const bool onBeat = std::fabs (n.start - std::round (n.start)) < 1e-6;
            const int vel = n.strong ? 106 : (onBeat ? 94 : 80);
            pattern.notes.push_back ({ std::clamp (pitches[i], 0, 127), n.start, n.length, vel, MelodyChannel });
        }
    }

    //==========================================================================
    // Chords & bass

    void emitChords()
    {
        static const std::vector<std::vector<double>> stabPatterns {
            { 0.0, 1.5, 2.5 }, { 0.0, 0.75, 2.0, 2.75 }, { 0.0, 1.0, 1.75, 3.0 }, { 0.0, 0.5, 2.0, 3.5 }
        };
        const auto& stabs = harmonyRng.pick (stabPatterns);

        for (auto& c : pattern.chords)
        {
            if (! style.chordStabs)
            {
                for (auto p : c.pitches)
                    pattern.notes.push_back ({ p, c.start, c.length - 0.02, 76, ChordChannel });
                continue;
            }

            for (double barStart = c.start; barStart < c.start + c.length - 1e-9; barStart += 4.0)
                for (auto t : stabs)
                    if (barStart + t < c.start + c.length - 1e-9)
                        for (auto p : c.pitches)
                            pattern.notes.push_back ({ p, barStart + t, 0.3, t == 0.0 ? 92 : 80, ChordChannel });
        }
    }

    void buildBass()
    {
        const double scale = 0.6 + 0.8 * density;

        for (auto& c : pattern.chords)
        {
            int root = 24 + wrap (set.rootPc + degreeToSemitone (steps, c.degree), 12);
            if (root < 28)
                root += 12;

            std::vector<double> hits { c.start };
            for (double barStart = c.start; barStart < c.start + c.length - 1e-9; barStart += 4.0)
                for (int s = 1; s < 16; ++s)
                {
                    const double t = barStart + s * 0.25;
                    if (t < c.start + c.length - 1e-9 && bassRng.uniform() < std::min (0.95, style.bassRhythm[(size_t) s] * scale))
                        hits.push_back (t);
                }
            if (c.length > 4.0 + 1e-9)
                hits.push_back (c.start + 4.0);

            std::sort (hits.begin(), hits.end());
            hits.erase (std::unique (hits.begin(), hits.end()), hits.end());

            for (size_t i = 0; i < hits.size(); ++i)
            {
                const double next = i + 1 < hits.size() ? hits[i + 1] : c.start + c.length;
                int pitch = root;
                if (i > 0 && bassRng.chance (style.bassOctaveJumpChance))
                    pitch += 12;
                else if (i > 0 && bassRng.chance (0.08))
                    {
                    pitch = 24 + wrap (set.rootPc + degreeToSemitone (steps, c.degree + 4), 12);
                    if (pitch < 28)
                        pitch += 12;
                }

                pattern.notes.push_back ({ pitch, hits[i], std::min (3.0, (next - hits[i]) * 0.95), i == 0 ? 110 : 100, BassChannel });
            }
        }
    }

    //==========================================================================

    void finalise()
    {
        const double len = pattern.lengthBeats();
        const double swing = std::clamp (set.swing, 0.0, 1.0);
        const double human = std::clamp (set.humanize, 0.0, 1.0);

        for (auto& n : pattern.notes)
        {
            const double sixteenth = n.start * 4.0;
            const double rounded = std::round (sixteenth);
            if (std::fabs (sixteenth - rounded) < 1e-6 && ((long) rounded) % 2 == 1)
                n.start += swing / 12.0;

            if (n.start > 1e-6)
                n.start += (feelRng.uniform() - 0.5) * human * 0.03;

            n.velocity = std::clamp ((int) std::lround (n.velocity + (feelRng.uniform() - 0.5) * human * 24.0), 1, 127);
            n.start = std::clamp (n.start, 0.0, len - 0.01);
            n.length = std::max (0.03, std::min (n.length, len - n.start - 0.001));
        }

        std::sort (pattern.notes.begin(), pattern.notes.end(), [] (const Note& a, const Note& b) {
            if (a.start != b.start)
                return a.start < b.start;
            if (a.channel != b.channel)
                return a.channel < b.channel;
            return a.pitch < b.pitch;
        });
    }
};

} // namespace

Pattern generate (const Settings& settings)
{
    return Builder (settings).build();
}

} // namespace basement
