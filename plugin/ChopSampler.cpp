#include "ChopSampler.h"

namespace
{

// 4-point Hermite interpolation; samples outside the buffer read as silence.
inline float readHermite (const float* d, int len, double x)
{
    const int i = (int) std::floor (x);
    const float f = (float) (x - i);
    auto at = [d, len] (int k) { return (k >= 0 && k < len) ? d[k] : 0.0f; };
    const float xm1 = at (i - 1), x0 = at (i), x1 = at (i + 1), x2 = at (i + 2);
    const float c1 = 0.5f * (x1 - xm1);
    const float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
    const float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
    return ((c3 * f + c2) * f + c1) * f + x0;
}

} // namespace

void ChopSampler::prepare (double sr)
{
    sampleRate = sr;
    triggers.reserve (2048);
    reset();
}

void ChopSampler::reset()
{
    for (auto& v : voices)
        v = Voice();
}

void ChopSampler::startRelease (Voice& v, double seconds)
{
    if (! v.active)
        return;
    const int len = std::max (1, (int) (seconds * sampleRate));
    if (v.releasing && v.release <= len)
        return;
    v.releasing = true;
    v.releaseLen = len;
    v.release = len;
}

void ChopSampler::releaseAll()
{
    for (auto& v : voices)
        startRelease (v, 0.01);
}

void ChopSampler::releaseNote (int note)
{
    for (auto& v : voices)
        if (v.active && v.note == note)
            startRelease (v, 0.015);
}

void ChopSampler::trigger (const ChopState& state, const basement::ChopEvent& e, double gateSamples, float pitchSemis, int note)
{
    const auto& sample = *state.sample;
    const int len = sample.getLength();
    if (len < 4 || state.slices.numSlices() == 0)
        return;

    // Choke whatever is playing (short crossfade, no click).
    for (auto& v : voices)
        startRelease (v, 0.004);

    Voice* voice = nullptr;
    for (auto& v : voices)
        if (! v.active)
        {
            voice = &v;
            break;
        }
    if (voice == nullptr)
    {
        voice = &voices[0];
        for (auto& v : voices)
            if (v.release < voice->release)
                voice = &v;
    }

    const int slice = ((e.slice % state.slices.numSlices()) + state.slices.numSlices()) % state.slices.numSlices();
    const int start = state.slices.starts[(size_t) slice];

    Voice v;
    v.active = true;
    v.inc = sample.sampleRate / sampleRate * std::pow (2.0, (e.semitones + pitchSemis) / 12.0);
    v.gain = e.gain;
    v.note = note;
    v.attackLen = std::max (1, (int) (0.002 * sampleRate));
    v.gateLeft = gateSamples >= 0 ? (int64_t) gateSamples : -1;

    if (e.reverse)
    {
        const double span = gateSamples >= 0 ? gateSamples * v.inc
                                             : (double) (state.slices.sliceEnd (slice, len) - start);
        v.pos = std::min ((double) len - 2.0, start + span);
        v.endPos = start;
        v.dir = -1;
    }
    else
    {
        v.pos = start;
        v.endPos = len - 1;
        v.dir = 1;
    }

    *voice = v;
}

void ChopSampler::triggerManual (const ChopState& state, int slice, float pitchSemis, float gain, int note)
{
    if (state.sample == nullptr)
        return;
    trigger (state, { 0.0, 0.0, slice, 0, false, gain }, -1.0, pitchSemis, note);
}

void ChopSampler::render (juce::AudioBuffer<float>& out, int start, int num, const ChopState& state)
{
    if (num <= 0)
        return;

    if (state.sample == nullptr)
    {
        reset();
        return;
    }

    const auto& audio = state.sample->audio;
    const int len = audio.getNumSamples();
    const int srcCh = audio.getNumChannels();
    const int outCh = out.getNumChannels();
    const float* src[2] = { audio.getReadPointer (0), audio.getReadPointer (srcCh > 1 ? 1 : 0) };
    const int fadeOut = std::max (1, (int) (0.004 * sampleRate));

    for (auto& v : voices)
    {
        if (! v.active)
            continue;

        for (int i = 0; i < num; ++i)
        {
            if (! v.releasing && v.gateLeft >= 0)
            {
                if (v.gateLeft == 0)
                    startRelease (v, 0.008);
                else
                    --v.gateLeft;
            }

            // Fade out before running off the end of the sample.
            const double remaining = (v.endPos - v.pos) * v.dir / v.inc;
            if (! v.releasing && remaining < fadeOut)
                startRelease (v, std::max (1.0, remaining) / sampleRate);

            float env = 1.0f;
            if (v.attack < v.attackLen)
                env *= (float) ++v.attack / (float) v.attackLen;
            if (v.releasing)
                env *= (float) v.release / (float) v.releaseLen;

            const float g = env * v.gain;
            const float l = readHermite (src[0], len, v.pos) * g;
            const float r = readHermite (src[1], len, v.pos) * g;
            out.addSample (0, start + i, outCh > 1 ? l : 0.5f * (l + r));
            if (outCh > 1)
                out.addSample (1, start + i, r);

            v.pos += v.inc * v.dir;

            if (v.releasing && --v.release <= 0)
            {
                v.active = false;
                break;
            }
        }
    }
}

int ChopSampler::processPattern (juce::AudioBuffer<float>& out, int start, int num, double startBeat, double beatsPerSample,
                                 const ChopState& state, float pitchSemis)
{
    const auto& events = state.pattern.events;
    if (state.sample == nullptr || events.empty())
    {
        render (out, start, num, state);
        return -1;
    }

    const double loop = state.pattern.lengthBeats();
    const double endBeat = startBeat + num * beatsPerSample;

    triggers.clear();
    for (size_t i = 0; i < events.size(); ++i)
    {
        const double t = events[i].start;
        for (double occ = t + std::ceil ((startBeat - t) / loop) * loop; occ < endBeat; occ += loop)
        {
            if (triggers.size() == triggers.capacity())
                break;
            triggers.push_back ({ juce::jlimit (0, num - 1, (int) ((occ - startBeat) / beatsPerSample)), i });
        }
    }
    std::sort (triggers.begin(), triggers.end(), [] (const Trigger& a, const Trigger& b) { return a.offset < b.offset; });

    int pos = 0, last = -1;
    for (auto& t : triggers)
    {
        if (t.offset > pos)
        {
            render (out, start + pos, t.offset - pos, state);
            pos = t.offset;
        }
        const auto& e = events[t.event];
        trigger (state, e, e.length / beatsPerSample, pitchSemis, -1);
        last = e.slice;
    }
    render (out, start + pos, num - pos, state);
    return last;
}
