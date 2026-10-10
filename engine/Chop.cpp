#include "Chop.h"
#include "Random.h"

#include <algorithm>
#include <cmath>
#include <map>

namespace basement
{

static std::vector<ChopStyleDef> buildChopStyles()
{
    std::vector<ChopStyleDef> s;

    s.push_back ({ "Flip",
                   "Classic flip: rearranged phrases in 3-3-2 pockets, keeps the sample's melody.",
                   2, { 2, 3, 4, 6, 8 }, { 3, 3, 3, 1.5, 1 },
                   0.15, 0.50, 0.20, 0.30, 0.05, 0.05, 0.05, 0, 4,
                   { "AABA'", "ABAB'", "AA'AA''" } });

    s.push_back ({ "Stutter",
                   "Glitchy jerk / new-wave chops: stutters, rolls, octave jumps.",
                   1, { 1, 2, 3, 4 }, { 3, 3, 2, 1 },
                   0.20, 0.30, 0.45, 0.25, 0.35, 0.25, 0.12, 0, 3,
                   { "AAAA'", "AA'AA'", "AAAB" } });

    s.push_back ({ "Slowed",
                   "Slowed, hazy long chops pitched down. Washed-out sample-flip feel.",
                   2, { 4, 6, 8, 12, 16 }, { 2, 2, 3, 1, 1 },
                   0.05, 0.65, 0.15, 0.20, 0.0, 0.02, 0.10, -4, 3,
                   { "AABA'", "AA'AA''" } });

    s.push_back ({ "Bounce",
                   "Bouncy rhythmic stabs with gaps for the 808 and hats.",
                   1, { 1, 2, 3 }, { 1, 3, 3 },
                   0.35, 0.30, 0.35, 0.35, 0.10, 0.10, 0.03, 0, 4,
                   { "AAAB", "AAAA'", "ABAB'" } });

    s.push_back ({ "Original",
                   "No chop: the sample in order, locked to your tempo. Use it to A/B.",
                   1, { 2 }, { 1 }, 0, 1, 0, 0, 0, 0, 0, 0, 1, { "A" } });

    return s;
}

static const std::vector<ChopStyleDef>& chopStyles()
{
    static const auto table = buildChopStyles();
    return table;
}

const ChopStyleDef& getChopStyle (ChopStyle s) { return chopStyles()[(size_t) s]; }
int numChopStyles() { return (int) chopStyles().size(); }

namespace
{

constexpr double stepBeats = 0.25;

struct Slot
{
    char motif;
    int variation;
};

std::vector<Slot> expandForm (const std::string& form, int numSlots)
{
    std::vector<Slot> base;
    for (char c : form)
    {
        if (c == '\'' && ! base.empty())
            ++base.back().variation;
        else if (c != '\'')
            base.push_back ({ c, 0 });
    }

    if (numSlots <= 1)
        return { { 'A', 0 } };
    if (numSlots == 2)
        return { { 'A', 0 }, { 'A', 1 } };

    std::vector<Slot> out;
    for (int i = 0; i < numSlots; ++i)
    {
        auto s = base[(size_t) (i % (int) base.size())];
        if (i >= (int) base.size() && i == numSlots - 1)
            s.variation += 1;
        out.push_back (s);
    }
    return out;
}

class ChopBuilder
{
public:
    explicit ChopBuilder (const ChopSettings& s)
        : set (s), style (getChopStyle (s.style)), rng ((uint64_t) s.seed * 0x2545F491ull + 17)
    {
        numSlices = std::max (1, s.numSlices);
        stepsPerSlice = std::max (1, s.stepsPerSlice);
        density = std::clamp (s.density, 0.0, 1.0);
        chaos = std::clamp (s.chaos, 0.0, 1.0);
        pattern.bars = std::clamp (s.bars, 1, 16);
    }

    ChopPattern build()
    {
        if (set.style == ChopStyle::Original)
            return buildOriginal();

        pickAnchors();

        const int motifBars = std::min (style.motifBars, pattern.bars);
        const int motifSteps = motifBars * 16;
        const int numSlots = std::max (1, pattern.bars / motifBars);
        const auto form = expandForm (rng.pick (style.forms), numSlots);

        std::map<std::pair<char, int>, std::vector<ChopEvent>> motifs;

        for (int k = 0; k < numSlots; ++k)
        {
            const auto key = std::make_pair (form[(size_t) k].motif, form[(size_t) k].variation);
            const auto baseKey = std::make_pair (key.first, 0);

            if (motifs.find (baseKey) == motifs.end())
                motifs[baseKey] = motif (0, motifSteps, -1, 0);

            if (motifs.find (key) == motifs.end())
            {
                // Variation: same first half (the "call"), new answer.
                std::vector<ChopEvent> v;
                int lastSlice = -1, lastLen = 0;
                for (auto& e : motifs[baseKey])
                    if (e.start < motifSteps * stepBeats * 0.5 - 1e-9)
                    {
                        v.push_back (e);
                        lastSlice = e.slice;
                        lastLen = (int) std::lround (e.length / stepBeats);
                    }
                for (auto& e : motif (motifSteps / 2, motifSteps, lastSlice, lastLen))
                    v.push_back (e);
                motifs[key] = v;
            }

            for (auto e : motifs[key])
            {
                e.start += k * motifSteps * stepBeats;
                pattern.events.push_back (e);
            }
        }

        std::sort (pattern.events.begin(), pattern.events.end(), [] (const ChopEvent& a, const ChopEvent& b) { return a.start < b.start; });
        return pattern;
    }

private:
    const ChopSettings& set;
    const ChopStyleDef& style;
    Rng rng;
    ChopPattern pattern;
    std::vector<int> anchors;
    int numSlices = 16, stepsPerSlice = 2;
    double density = 0.5, chaos = 0.3;

    ChopPattern buildOriginal()
    {
        const int totalSteps = pattern.bars * 16;
        for (int step = 0, i = 0; step < totalSteps; step += stepsPerSlice, ++i)
            pattern.events.push_back ({ step * stepBeats, stepsPerSlice * stepBeats, i % numSlices, 0, false, 1.0f });
        return pattern;
    }

    void pickAnchors()
    {
        // Anchors are the few chops a pattern keeps coming back to; that is
        // what makes a flip sound like a hook rather than random slices.
        for (int tries = 0; (int) anchors.size() < std::min (style.numAnchors, numSlices) && tries < 100; ++tries)
        {
            int a = rng.range (0, numSlices - 1);
            if (stepsPerSlice < 4 && (a % 2) == 1 && rng.chance (0.7))
                a -= 1; // prefer slices that start on a beat of the sample
            if (std::find (anchors.begin(), anchors.end(), a) == anchors.end())
                anchors.push_back (a);
        }
        if (anchors.empty())
            anchors.push_back (0);
    }

    int chooseLength (int remaining)
    {
        std::vector<double> w = style.lengthWeights;
        for (size_t i = 0; i < w.size(); ++i)
        {
            const int len = style.lengths[i];
            if (density > 0.5 && len <= 2)
                w[i] *= 1.0 + (density - 0.5) * 3.0;
            if (density < 0.5 && len >= 4)
                w[i] *= 1.0 + (0.5 - density) * 3.0;
        }
        return std::min (remaining, style.lengths[(size_t) rng.weighted (w)]);
    }

    int chooseSlice (int prevSlice, int prevLen)
    {
        if (prevSlice < 0)
            return rng.pick (anchors);

        const double total = style.continueProb + style.repeatProb + style.jumpProb;
        const double r = rng.uniform() * total;

        if (r < style.continueProb)
            return (prevSlice + std::max (1, prevLen / stepsPerSlice)) % numSlices;
        if (r < style.continueProb + style.repeatProb)
            return prevSlice;

        int a = rng.pick (anchors);
        if (rng.chance (0.3))
            a = (a + 1) % numSlices;
        return a;
    }

    std::vector<ChopEvent> motif (int fromStep, int toStep, int prevSlice, int prevLen)
    {
        std::vector<ChopEvent> out;
        int t = fromStep;

        while (t < toStep)
        {
            const bool onBeat = t % 4 == 0;
            if (t > fromStep && ! onBeat && rng.chance (style.restChance * (1.3 - density)))
            {
                t += rng.chance (0.5) ? 1 : 2;
                continue;
            }

            const int len = chooseLength (toStep - t);
            const int slice = chooseSlice (prevSlice, prevLen);

            int semis = style.semitoneOffset;
            if (rng.chance (style.octaveChance * chaos * 2.0))
                semis += rng.chance (0.7) ? 12 : -12;
            const bool reverse = rng.chance (style.reverseChance * chaos * 2.0);

            if (len >= 2 && rng.chance (style.rollChance * (0.5 + chaos)))
            {
                // Roll: retrigger the same chop on 1/16s (1/32s when chaotic).
                const double sub = chaos > 0.6 && rng.chance (0.5) ? stepBeats * 0.5 : stepBeats;
                const int hits = (int) std::lround (len * stepBeats / sub);
                for (int h = 0; h < hits; ++h)
                {
                    const float g = 0.65f + 0.35f * (float) (h + 1) / (float) hits;
                    out.push_back ({ t * stepBeats + h * sub, sub, slice, semis, false, g });
                }
            }
            else
            {
                const float g = onBeat ? 1.0f : 0.9f;
                out.push_back ({ t * stepBeats, len * stepBeats, slice, semis, reverse, g });
            }

            prevSlice = slice;
            prevLen = len;
            t += len;
        }
        return out;
    }
};

} // namespace

ChopPattern generateChops (const ChopSettings& settings)
{
    return ChopBuilder (settings).build();
}

} // namespace basement
