// Command-line front end for the melody engine: renders .mid files without a DAW.
//
//   basement-cli --style rage --key F# --bars 8 --seed 42 -o rage.mid

#include "Generator.h"
#include "MidiWriter.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

using namespace basement;

static void usage()
{
    std::printf ("usage: basement-cli [options]\n"
                 "  --style NAME      ");
    for (int i = 0; i < numStyles(); ++i)
        std::printf ("%s%s", i ? ", " : "", getStyle ((StyleId) i).name.c_str());
    std::printf ("\n"
                 "  --key NOTE        root note, e.g. C, F#, Bb      (default C)\n"
                 "  --scale N         0..%d to override the style's scale\n"
                 "  --bars N          1, 2, 4 or 8                    (default 4)\n"
                 "  --seed N                                          (default 1)\n"
                 "  --density X       0..1                            (default 0.5)\n"
                 "  --darkness X      0..1                            (default 0.5)\n"
                 "  --swing X         0..1                            (default 0)\n"
                 "  --humanize X      0..1                            (default 0.3)\n"
                 "  --octave N        melody octave shift, -2..2\n"
                 "  --bpm N           tempo written to the file (default: style tempo)\n"
                 "  --no-melody | --no-chords | --no-bass\n"
                 "  -o FILE           output .mid (default basement.mid)\n",
                 numScales() - 1);
}

int main (int argc, char** argv)
{
    Settings s;
    std::string out = "basement.mid";
    double bpm = 0;

    for (int i = 1; i < argc; ++i)
    {
        const std::string a = argv[i];
        auto value = [&]() -> std::string {
            if (i + 1 >= argc)
            {
                std::fprintf (stderr, "missing value for %s\n", a.c_str());
                std::exit (1);
            }
            return argv[++i];
        };

        if (a == "--style")
        {
            bool ok;
            s.style = parseStyle (value(), ok);
            if (! ok)
            {
                std::fprintf (stderr, "unknown style\n");
                return 1;
            }
        }
        else if (a == "--key")
        {
            s.rootPc = parseNoteName (value());
            if (s.rootPc < 0)
            {
                std::fprintf (stderr, "bad key\n");
                return 1;
            }
        }
        else if (a == "--scale")    s.scaleOverride = std::atoi (value().c_str());
        else if (a == "--bars")     s.bars = std::atoi (value().c_str());
        else if (a == "--seed")     s.seed = (uint32_t) std::strtoul (value().c_str(), nullptr, 10);
        else if (a == "--density")  s.density = std::atof (value().c_str());
        else if (a == "--darkness") s.darkness = std::atof (value().c_str());
        else if (a == "--swing")    s.swing = std::atof (value().c_str());
        else if (a == "--humanize") s.humanize = std::atof (value().c_str());
        else if (a == "--octave")   s.octaveShift = std::atoi (value().c_str());
        else if (a == "--bpm")      bpm = std::atof (value().c_str());
        else if (a == "--no-melody") s.melody = false;
        else if (a == "--no-chords") s.chords = false;
        else if (a == "--no-bass")   s.bass = false;
        else if (a == "-o")          out = value();
        else
        {
            usage();
            return a == "-h" || a == "--help" ? 0 : 1;
        }
    }

    const auto& style = getStyle (s.style);
    if (bpm <= 0)
        bpm = style.defaultTempo;

    const auto p = generate (s);

    std::printf ("%s in %s, %d bars, seed %u, %.0f bpm\n", style.name.c_str(), noteName (s.rootPc).c_str(), p.bars, s.seed, bpm);
    std::printf ("chords: %s\n", p.chordSummary().c_str());
    std::printf ("melody:");
    for (auto& n : p.notes)
        if (n.channel == MelodyChannel)
            std::printf (" %s", midiNoteName (n.pitch).c_str());
    std::printf ("\n");

    if (! saveMidiFile (out, p, bpm))
    {
        std::fprintf (stderr, "could not write %s\n", out.c_str());
        return 1;
    }
    std::printf ("wrote %s (%zu notes)\n", out.c_str(), p.notes.size());
    return 0;
}
