#include "MidiWriter.h"

#include <algorithm>
#include <cmath>
#include <fstream>

namespace basement
{

namespace
{

constexpr int ppq = 960;

struct Event
{
    long tick;
    int order; // note-offs before note-ons at the same tick
    std::vector<uint8_t> bytes;
};

void put32 (std::vector<uint8_t>& out, uint32_t v)
{
    for (int s = 24; s >= 0; s -= 8)
        out.push_back ((uint8_t) (v >> s));
}

void put16 (std::vector<uint8_t>& out, uint16_t v)
{
    out.push_back ((uint8_t) (v >> 8));
    out.push_back ((uint8_t) v);
}

void putVarLen (std::vector<uint8_t>& out, uint32_t v)
{
    uint8_t buf[5];
    int n = 0;
    buf[n++] = (uint8_t) (v & 0x7f);
    while ((v >>= 7) > 0)
        buf[n++] = (uint8_t) ((v & 0x7f) | 0x80);
    while (n > 0)
        out.push_back (buf[--n]);
}

void putTrack (std::vector<uint8_t>& file, std::vector<Event> events, long endTick)
{
    std::stable_sort (events.begin(), events.end(), [] (const Event& a, const Event& b) {
        return a.tick != b.tick ? a.tick < b.tick : a.order < b.order;
    });

    std::vector<uint8_t> data;
    long last = 0;
    for (auto& e : events)
    {
        putVarLen (data, (uint32_t) (e.tick - last));
        data.insert (data.end(), e.bytes.begin(), e.bytes.end());
        last = e.tick;
    }

    putVarLen (data, (uint32_t) std::max (0L, endTick - last));
    data.insert (data.end(), { 0xFF, 0x2F, 0x00 });

    file.insert (file.end(), { 'M', 'T', 'r', 'k' });
    put32 (file, (uint32_t) data.size());
    file.insert (file.end(), data.begin(), data.end());
}

Event trackName (const std::string& name)
{
    Event e { 0, -1, { 0xFF, 0x03 } };
    putVarLen (e.bytes, (uint32_t) name.size());
    e.bytes.insert (e.bytes.end(), name.begin(), name.end());
    return e;
}

} // namespace

std::vector<uint8_t> writeMidiFile (const Pattern& pattern, double bpm, int layers)
{
    struct Layer
    {
        int mask;
        int channel;
        const char* name;
    };
    const Layer all[] = { { LayerMelody, MelodyChannel, "Melody" },
                          { LayerChords, ChordChannel, "Chords" },
                          { LayerBass, BassChannel, "808" } };

    const long endTick = (long) std::lround (pattern.lengthBeats() * ppq);

    std::vector<std::vector<Event>> tracks;
    for (auto& layer : all)
    {
        if ((layers & layer.mask) == 0)
            continue;

        std::vector<Event> ev { trackName (layer.name) };
        const auto ch = (uint8_t) (layer.channel - 1);

        for (auto& n : pattern.notes)
        {
            if (n.channel != layer.channel)
                continue;

            const long on = (long) std::lround (n.start * ppq);
            const long off = std::max (on + 1, (long) std::lround ((n.start + n.length) * ppq));
            ev.push_back ({ on, 1, { (uint8_t) (0x90 | ch), (uint8_t) n.pitch, (uint8_t) n.velocity } });
            ev.push_back ({ off, 0, { (uint8_t) (0x80 | ch), (uint8_t) n.pitch, 0 } });
        }
        tracks.push_back (std::move (ev));
    }

    std::vector<uint8_t> file { 'M', 'T', 'h', 'd' };
    put32 (file, 6);
    put16 (file, 1);
    put16 (file, (uint16_t) (tracks.size() + 1));
    put16 (file, ppq);

    // Tempo track
    const auto usPerBeat = (uint32_t) std::lround (60000000.0 / std::clamp (bpm, 20.0, 400.0));
    std::vector<Event> tempo { trackName ("Basement") };
    tempo.push_back ({ 0, 0, { 0xFF, 0x51, 0x03, (uint8_t) (usPerBeat >> 16), (uint8_t) (usPerBeat >> 8), (uint8_t) usPerBeat } });
    tempo.push_back ({ 0, 0, { 0xFF, 0x58, 0x04, 4, 2, 24, 8 } });
    putTrack (file, tempo, endTick);

    for (auto& t : tracks)
        putTrack (file, t, endTick);

    return file;
}

bool saveMidiFile (const std::string& path, const Pattern& pattern, double bpm, int layers)
{
    const auto bytes = writeMidiFile (pattern, bpm, layers);
    std::ofstream f (path, std::ios::binary);
    if (! f)
        return false;
    f.write ((const char*) bytes.data(), (std::streamsize) bytes.size());
    return (bool) f;
}

} // namespace basement
