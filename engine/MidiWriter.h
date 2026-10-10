#pragma once

#include "Generator.h"

#include <cstdint>
#include <string>
#include <vector>

namespace basement
{

enum LayerMask
{
    LayerMelody = 1 << 0,
    LayerChords = 1 << 1,
    LayerBass = 1 << 2,
    LayerAll = LayerMelody | LayerChords | LayerBass
};

// Standard MIDI File (type 1, 960 PPQ): a tempo track plus one track per layer,
// so a DAW splits Melody / Chords / 808 onto separate lanes when dropped in.
std::vector<uint8_t> writeMidiFile (const Pattern& pattern, double bpm, int layers = LayerAll);

bool saveMidiFile (const std::string& path, const Pattern& pattern, double bpm, int layers = LayerAll);

} // namespace basement
