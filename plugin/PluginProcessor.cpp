#include "PluginProcessor.h"
#include "MidiWriter.h"
#include "PluginEditor.h"

namespace
{

const juce::StringArray barChoices { "1", "2", "4", "8" };
const juce::StringArray octaveChoices { "-2", "-1", "0", "+1", "+2" };

const char* const regenerateParams[] = { "style", "key", "scale", "bars", "density", "darkness",
                                         "swing", "humanize", "octave", "melody", "chords", "bass" };

juce::StringArray styleNames()
{
    juce::StringArray s;
    for (int i = 0; i < basement::numStyles(); ++i)
        s.add (basement::getStyle ((basement::StyleId) i).name);
    return s;
}

juce::StringArray scaleNames()
{
    juce::StringArray s { "Style Default" };
    for (int i = 0; i < basement::numScales(); ++i)
        s.add (basement::getScale ((basement::ScaleId) i).name);
    return s;
}

juce::StringArray keyNames()
{
    juce::StringArray s;
    for (auto& n : basement::noteNames())
        s.add (n);
    return s;
}

} // namespace

BasementProcessor::BasementProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "BasementState", createLayout())
{
    for (auto* id : regenerateParams)
        apvts.addParameterListener (id, this);

    seed = (uint32_t) juce::Random::getSystemRandom().nextInt (1 << 20) + 1;
    scratchEvents.reserve (4096);
    regenerate();
}

BasementProcessor::~BasementProcessor()
{
    cancelPendingUpdate();
    for (auto* id : regenerateParams)
        apvts.removeParameterListener (id, this);
}

juce::AudioProcessorValueTreeState::ParameterLayout BasementProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto pct = AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (juce::roundToInt (v * 100.0f)) + "%"; });

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "style", 1 }, "Style", styleNames(), 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "key", 1 }, "Key", keyNames(), 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "scale", 1 }, "Scale", scaleNames(), 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "bars", 1 }, "Bars", barChoices, 2));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "octave", 1 }, "Octave", octaveChoices, 2));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "density", 1 }, "Density", NormalisableRange<float> (0.0f, 1.0f), 0.5f, pct));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "darkness", 1 }, "Darkness", NormalisableRange<float> (0.0f, 1.0f), 0.5f, pct));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "swing", 1 }, "Swing", NormalisableRange<float> (0.0f, 1.0f), 0.0f, pct));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "humanize", 1 }, "Humanize", NormalisableRange<float> (0.0f, 1.0f), 0.3f, pct));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "melody", 1 }, "Melody", true));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "chords", 1 }, "Chords", true));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "bass", 1 }, "808", true));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "sound", 1 }, "Preview Sound", true));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "tempo", 1 }, "Preview Tempo",
                                                       NormalisableRange<float> (60.0f, 200.0f, 1.0f), 140.0f,
                                                       AudioParameterFloatAttributes().withLabel ("bpm")));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "volume", 1 }, "Preview Volume", NormalisableRange<float> (0.0f, 1.0f), 0.7f, pct));

    return layout;
}

basement::Settings BasementProcessor::currentSettings() const
{
    auto choice = [this] (const char* id) { return juce::roundToInt (apvts.getRawParameterValue (id)->load()); };
    auto value = [this] (const char* id) { return (double) apvts.getRawParameterValue (id)->load(); };
    auto flag = [this] (const char* id) { return apvts.getRawParameterValue (id)->load() > 0.5f; };

    basement::Settings s;
    s.style = (basement::StyleId) choice ("style");
    s.rootPc = choice ("key");
    s.scaleOverride = choice ("scale") - 1;
    s.bars = barChoices[choice ("bars")].getIntValue();
    s.octaveShift = choice ("octave") - 2;
    s.density = value ("density");
    s.darkness = value ("darkness");
    s.swing = value ("swing");
    s.humanize = value ("humanize");
    s.melody = flag ("melody");
    s.chords = flag ("chords");
    s.bass = flag ("bass");
    s.seed = seed;
    return s;
}

void BasementProcessor::regenerate()
{
    auto pattern = std::make_shared<basement::Pattern> (basement::generate (currentSettings()));
    auto forAudio = std::make_unique<basement::Pattern> (*pattern);

    std::unique_ptr<basement::Pattern> old; // freed here, never on the audio thread
    {
        const juce::SpinLock::ScopedLockType lock (patternLock);
        old = std::move (pendingPattern);
        pendingPattern = std::move (forAudio);
        hasPendingPattern = true;
    }

    uiPattern = pattern;
    ++patternVersion;
}

void BasementProcessor::randomiseSeed()
{
    seed = (uint32_t) juce::Random::getSystemRandom().nextInt (1 << 20) + 1;
    regenerate();
}

void BasementProcessor::nudgeSeed (int delta)
{
    seed = (uint32_t) juce::jmax (1, (int) seed.load() + delta);
    regenerate();
}

void BasementProcessor::parameterChanged (const juce::String&, float)
{
    // May arrive from the audio thread (automation); regenerate on the message thread.
    triggerAsyncUpdate();
}

void BasementProcessor::handleAsyncUpdate() { regenerate(); }

//==============================================================================
void BasementProcessor::prepareToPlay (double sr, int)
{
    sampleRate = sr;
    previewSynth.prepare (sr);
    for (auto& ch : activeNotes)
        ch.reset();
    wasPlaying = false;
}

bool BasementProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void BasementProcessor::setInternalPlaying (bool shouldPlay)
{
    if (shouldPlay && ! internalPlaying)
        internalRestart = true;
    internalPlaying = shouldPlay;
}

void BasementProcessor::flushActiveNotes (juce::MidiBuffer& out, int sample)
{
    for (int ch = 0; ch < 16; ++ch)
    {
        if (activeNotes[(size_t) ch].none())
            continue;
        for (int p = 0; p < 128; ++p)
            if (activeNotes[(size_t) ch][(size_t) p])
                out.addEvent (juce::MidiMessage::noteOff (ch + 1, p), sample);
        activeNotes[(size_t) ch].reset();
    }
}

void BasementProcessor::renderPattern (juce::MidiBuffer& out, double startBeat, double beatsPerSample, int numSamples)
{
    const auto& p = *playingPattern;
    const double loop = p.lengthBeats();
    const double endBeat = startBeat + numSamples * beatsPerSample;

    scratchEvents.clear();

    auto schedule = [&] (double t, bool on, const basement::Note& n) {
        double occ = t + std::ceil ((startBeat - t) / loop) * loop;
        for (; occ < endBeat; occ += loop)
        {
            if (scratchEvents.size() == scratchEvents.capacity())
                return;
            const int s = juce::jlimit (0, numSamples - 1, (int) ((occ - startBeat) / beatsPerSample));
            scratchEvents.push_back ({ s, on, n.channel, n.pitch, n.velocity });
        }
    };

    for (auto& n : p.notes)
    {
        schedule (n.start, true, n);
        schedule (n.start + n.length, false, n);
    }

    std::sort (scratchEvents.begin(), scratchEvents.end(), [] (const PendingEvent& a, const PendingEvent& b) {
        return a.sample != b.sample ? a.sample < b.sample : (! a.on && b.on);
    });

    for (auto& e : scratchEvents)
    {
        auto& active = activeNotes[(size_t) (e.channel - 1)];
        if (e.on)
        {
            if (active[(size_t) e.pitch])
                out.addEvent (juce::MidiMessage::noteOff (e.channel, e.pitch), e.sample);
            out.addEvent (juce::MidiMessage::noteOn (e.channel, e.pitch, (juce::uint8) e.velocity), e.sample);
            active[(size_t) e.pitch] = true;
        }
        else if (active[(size_t) e.pitch])
        {
            out.addEvent (juce::MidiMessage::noteOff (e.channel, e.pitch), e.sample);
            active[(size_t) e.pitch] = false;
        }
    }
}

void BasementProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    generatedMidi.clear();

    const int numSamples = buffer.getNumSamples();

    {
        const juce::SpinLock::ScopedTryLockType lock (patternLock);
        if (lock.isLocked() && hasPendingPattern)
        {
            std::swap (pendingPattern, playingPattern);
            hasPendingPattern = false;
            flushActiveNotes (generatedMidi, 0);
        }
    }

    // Transport: follow the host when it is playing, otherwise the internal clock.
    double bpm = apvts.getRawParameterValue ("tempo")->load();
    bool playing = false;
    double startBeat = 0.0;

    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm())
            {
                bpm = *b;
                hostBpm = *b;
            }

            if (pos->getIsPlaying())
            {
                if (auto ppq = pos->getPpqPosition())
                {
                    playing = true;
                    startBeat = *ppq;
                }
            }
        }
    }
    hostPlaying = playing;

    if (! playing && internalPlaying)
    {
        if (internalRestart.exchange (false))
            internalBeat = 0.0;
        playing = true;
        startBeat = internalBeat;
    }

    const double beatsPerSample = juce::jmax (1.0, bpm) / 60.0 / sampleRate;

    if (playing && playingPattern != nullptr)
    {
        if (! wasPlaying || std::abs (startBeat - expectedBeat) > 0.01)
            flushActiveNotes (generatedMidi, 0); // started or jumped

        renderPattern (generatedMidi, startBeat, beatsPerSample, numSamples);

        expectedBeat = startBeat + numSamples * beatsPerSample;
        internalBeat = expectedBeat;
        const double loop = playingPattern->lengthBeats();
        playheadBeat = startBeat - std::floor (startBeat / loop) * loop;
    }
    else
    {
        if (wasPlaying)
            flushActiveNotes (generatedMidi, 0);
        playheadBeat = -1.0;
    }
    wasPlaying = playing;

    // Pass incoming MIDI through alongside the generated notes.
    generatedMidi.addEvents (midi, 0, numSamples, 0);
    midi.swapWith (generatedMidi);

    if (apvts.getRawParameterValue ("sound")->load() > 0.5f)
    {
        previewSynth.render (buffer, midi);
        buffer.applyGain (apvts.getRawParameterValue ("volume")->load());
    }
    else
    {
        previewSynth.allNotesOff();
    }
}

//==============================================================================
double BasementProcessor::getExportTempo() const
{
    const double host = hostBpm.load();
    return host > 0 ? host : (double) apvts.getRawParameterValue ("tempo")->load();
}

juce::String BasementProcessor::suggestedFileName (int layers) const
{
    const auto s = currentSettings();
    juce::String layer = "Full";
    if (layers == basement::LayerMelody)
        layer = "Melody";
    else if (layers == basement::LayerChords)
        layer = "Chords";
    else if (layers == basement::LayerBass)
        layer = "808";

    const auto style = juce::String (basement::getStyle (s.style).name).removeCharacters (" /");
    return "Basement_" + style + "_" + juce::String (basement::noteName (s.rootPc)).replace ("#", "s") + "_"
           + juce::String (juce::roundToInt (getExportTempo())) + "bpm_" + juce::String (s.seed) + "_" + layer + ".mid";
}

juce::MemoryBlock BasementProcessor::midiData (int layers) const
{
    const auto bytes = basement::writeMidiFile (*uiPattern, getExportTempo(), layers);
    return juce::MemoryBlock (bytes.data(), bytes.size());
}

juce::File BasementProcessor::writeMidiToTempFile (int layers) const
{
    auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("Basement");
    dir.createDirectory();
    auto file = dir.getChildFile (suggestedFileName (layers));
    const auto data = midiData (layers);
    file.replaceWithData (data.getData(), data.getSize());
    return file;
}

//==============================================================================
void BasementProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("seed", (int) seed.load(), nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void BasementProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto state = juce::ValueTree::fromXml (*xml);
            seed = (uint32_t) juce::jmax (1, (int) state.getProperty ("seed", 1));
            apvts.replaceState (state);
            triggerAsyncUpdate();
        }
    }
}

juce::AudioProcessorEditor* BasementProcessor::createEditor() { return new BasementEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BasementProcessor(); }
