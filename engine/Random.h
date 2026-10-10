#pragma once

#include <cstdint>
#include <vector>

namespace basement
{

// Small deterministic RNG (splitmix64). std::uniform_*_distribution is not
// portable across standard libraries, and the same seed must produce the same
// melody on macOS, Windows and Linux, so everything is derived from raw bits.
class Rng
{
public:
    explicit Rng (uint64_t seed) : state (seed * 0x9E3779B97F4A7C15ull + 0x632BE59BD9B4E019ull) {}

    uint64_t next()
    {
        uint64_t z = (state += 0x9E3779B97F4A7C15ull);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
        return z ^ (z >> 31);
    }

    // [0, 1)
    double uniform() { return (double) (next() >> 11) * (1.0 / 9007199254740992.0); }

    bool chance (double p) { return uniform() < p; }

    // [lo, hi]
    int range (int lo, int hi)
    {
        if (hi <= lo)
            return lo;
        return lo + (int) (next() % (uint64_t) (hi - lo + 1));
    }

    template <typename T>
    const T& pick (const std::vector<T>& v) { return v[(size_t) range (0, (int) v.size() - 1)]; }

    int weighted (const std::vector<double>& weights)
    {
        double total = 0;
        for (auto w : weights)
            total += w;

        double r = uniform() * total;
        for (size_t i = 0; i < weights.size(); ++i)
        {
            r -= weights[i];
            if (r < 0)
                return (int) i;
        }
        return (int) weights.size() - 1;
    }

private:
    uint64_t state;
};

} // namespace basement
