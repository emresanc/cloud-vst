#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{

const juce::StringArray barChoices { "1", "2", "4", "8" };
const juce::StringArray chopChoices { "1/4", "1/8", "1/16", "Transient" };
const juce::StringArray sampleBarChoices { "Auto", "1", "2", "4", "8", "16" };

const char* const rebuildParams[] = { "style", "bars", "chop", "samplebars", "density", "chaos" };

juce::StringArray chopStyleNames()
{
    juce::StringArray s;
    for (int i = 0; i < basement::numChopStyles(); ++i)
        s.add (basement::getChopStyle ((basement::ChopStyle) i).name);
    return s;
}

} // namespace

BasementProcessor::BasementProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "BasementState", createLayout())
{
    for (auto* id : rebuildParams)
        apvts.addParameterListener (id, this);

    seed = (uint32_t) juce::Random::getSystemRandom().nextInt (1 << 20) + 1;
    rebuild();
}

BasementProcessor::~BasementProcessor()
{
    cancelPendingUpdate();
    for (auto* id : rebuildParams)
        apvts.removeParameterListener (id, this);
}

juce::AudioProcessorValueTreeState::ParameterLayout BasementProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto pct = AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v * 100.0f)) + "%"; });
    auto hz = AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) {
        return v >= 19999.0f ? String ("Open") : v >= 1000.0f ? String (v / 1000.0f, 1) + "k" : String (roundToInt (v));
    });
    auto semis = AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) {
        const int s = roundToInt (v);
        return (s > 0 ? "+" : "") + String (s) + " st";
    });

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "style", 2 }, "Chop Style", chopStyleNames(), 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "bars", 2 }, "Pattern Bars", barChoices, 2));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "chop", 2 }, "Chop Size", chopChoices, 1));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "samplebars", 2 }, "Sample Bars", sampleBarChoices, 0));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "pitch", 2 }, "Pitch", NormalisableRange<float> (-12.0f, 12.0f, 1.0f), 0.0f, semis));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "density", 2 }, "Density", NormalisableRange<float> (0.0f, 1.0f), 0.5f, pct));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "chaos", 2 }, "Chaos", NormalisableRange<float> (0.0f, 1.0f), 0.3f, pct));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "tone", 2 }, "Tone", NormalisableRange<float> (300.0f, 20000.0f, 0.0f, 0.3f), 20000.0f, hz));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "tempo", 2 }, "Preview Tempo",
                                                       NormalisableRange<float> (60.0f, 200.0f, 1.0f), 140.0f,
                                                       AudioParameterFloatAttributes().withLabel ("bpm")));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "volume", 2 }, "Volume", NormalisableRange<float> (0.0f, 1.0f), 0.8f, pct));

    return layout;
}

//==============================================================================
bool BasementProcessor::loadSample (const juce::File& file, juce::String& error)
{
    auto s = loadSampleFile (file, error);
    if (s == nullptr)
        return false;
    setSample (s);
    return true;
}

void BasementProcessor::setSample (std::shared_ptr<SampleAudio> s)
{
    sample = std::move (s);
    rebuild();
}

void BasementProcessor::rebuild()
{
    auto choice = [this] (const char* id) { return juce::roundToInt (apvts.getRawParameterValue (id)->load()); };
    auto value = [this] (const char* id) { return (double) apvts.getRawParameterValue (id)->load(); };

    auto state = std::make_shared<ChopState>();
    state->sample = sample;

    if (sample != nullptr)
    {
        const int sampleBarsIdx = choice ("samplebars");
        state->slices = sliceSample (*sample, sampleBarsIdx == 0 ? 0 : sampleBarChoices[sampleBarsIdx].getIntValue(),
                                     (ChopSize) choice ("chop"));

        basement::ChopSettings cs;
        cs.style = (basement::ChopStyle) choice ("style");
        cs.bars = barChoices[choice ("bars")].getIntValue();
        cs.numSlices = state->slices.numSlices();
        cs.stepsPerSlice = state->slices.stepsPerSlice;
        cs.density = value ("density");
        cs.chaos = value ("chaos");
        cs.seed = seed;
        state->pattern = basement::generateChops (cs);
    }

    auto forAudio = std::make_unique<ChopState> (*state);
    std::unique_ptr<ChopState> old; // freed here, never on the audio thread
    {
        const juce::SpinLock::ScopedLockType lock (stateLock);
        old = std::move (pendingState);
        pendingState = std::move (forAudio);
        hasPendingState = true;
    }

    uiState = state;
    ++stateVersion;
}

void BasementProcessor::randomiseSeed()
{
    seed = (uint32_t) juce::Random::getSystemRandom().nextInt (1 << 20) + 1;
    rebuild();
}

void BasementProcessor::nudgeSeed (int delta)
{
    seed = (uint32_t) juce::jmax (1, (int) seed.load() + delta);
    rebuild();
}

void BasementProcessor::parameterChanged (const juce::String&, float) { triggerAsyncUpdate(); }
void BasementProcessor::handleAsyncUpdate() { rebuild(); }

//==============================================================================
void BasementProcessor::prepareToPlay (double sr, int)
{
    sampleRate = sr;
    sampler.prepare (sr);
    currentTone = -1.0f;
    for (auto& f : toneFilter)
        f.reset();
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

void BasementProcessor::updateTone (double sr)
{
    const float tone = apvts.getRawParameterValue ("tone")->load();
    if (std::abs (tone - currentTone) < 1.0f)
        return;
    currentTone = tone;
    const auto coeffs = juce::IIRCoefficients::makeLowPass (sr, juce::jmin ((double) tone, sr * 0.45), 0.8);
    for (auto& f : toneFilter)
        f.setCoefficients (coeffs);
}

void BasementProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    const int numSamples = buffer.getNumSamples();

    {
        const juce::SpinLock::ScopedTryLockType lock (stateLock);
        if (lock.isLocked() && hasPendingState)
        {
            std::swap (pendingState, playingState);
            hasPendingState = false;
        }
    }

    if (playingState == nullptr)
        return;

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
                if (auto ppq = pos->getPpqPosition())
                {
                    playing = true;
                    startBeat = *ppq;
                }
        }
    }

    if (! playing && internalPlaying)
    {
        if (internalRestart.exchange (false))
            internalBeat = 0.0;
        playing = true;
        startBeat = internalBeat;
    }

    const double beatsPerSample = juce::jmax (1.0, bpm) / 60.0 / sampleRate;
    const float pitch = apvts.getRawParameterValue ("pitch")->load();
    const auto& state = *playingState;

    if (playing && ! wasPlaying)
        sampler.releaseAll();
    if (! playing && wasPlaying)
        sampler.releaseAll();

    // MIDI: C1 and up play the slices directly, split the block at each event.
    int pos = 0;
    for (const auto m : midi)
    {
        const auto msg = m.getMessage();
        const int at = juce::jlimit (0, numSamples, m.samplePosition);

        if (at > pos)
        {
            if (playing)
            {
                const int s = sampler.processPattern (buffer, pos, at - pos, startBeat + pos * beatsPerSample, beatsPerSample, state, pitch);
                if (s >= 0)
                    lastSlice = s;
            }
            else
                sampler.render (buffer, pos, at - pos, state);
            pos = at;
        }

        if (msg.isNoteOn() && msg.getNoteNumber() >= firstSliceNote)
        {
            const int slice = msg.getNoteNumber() - firstSliceNote;
            if (slice < state.slices.numSlices())
            {
                sampler.triggerManual (state, slice, pitch, msg.getFloatVelocity(), msg.getNoteNumber());
                lastSlice = slice;
            }
        }
        else if (msg.isNoteOff())
            sampler.releaseNote (msg.getNoteNumber());
    }

    if (pos < numSamples)
    {
        if (playing)
        {
            const int s = sampler.processPattern (buffer, pos, numSamples - pos, startBeat + pos * beatsPerSample, beatsPerSample, state, pitch);
            if (s >= 0)
                lastSlice = s;
        }
        else
            sampler.render (buffer, pos, numSamples - pos, state);
    }

    if (playing)
    {
        if (wasPlaying && std::abs (startBeat - expectedBeat) > 0.01)
            sampler.releaseAll(); // jumped
        expectedBeat = startBeat + numSamples * beatsPerSample;
        internalBeat = expectedBeat;
        const double loop = state.pattern.lengthBeats();
        playheadBeat = startBeat - std::floor (startBeat / loop) * loop;
    }
    else
    {
        playheadBeat = -1.0;
    }
    wasPlaying = playing;
    midi.clear();

    updateTone (sampleRate);
    if (currentTone < 19999.0f)
        for (int ch = 0; ch < juce::jmin (2, buffer.getNumChannels()); ++ch)
            toneFilter[ch].processSamples (buffer.getWritePointer (ch), numSamples);

    buffer.applyGain (apvts.getRawParameterValue ("volume")->load());
}

//==============================================================================
double BasementProcessor::getTempo() const
{
    const double host = hostBpm.load();
    return host > 0 ? host : (double) apvts.getRawParameterValue ("tempo")->load();
}

void BasementProcessor::renderLoop (juce::AudioBuffer<float>& out, double sr, double bpm) const
{
    auto state = uiState;
    const double loop = state->pattern.lengthBeats();
    const int total = (int) std::ceil (loop * 60.0 / bpm * sr);
    out.setSize (2, total);
    out.clear();
    if (state->sample == nullptr)
        return;

    // Play the loop twice and keep the second pass, so chops that ring over
    // the loop point are already sounding at the start (seamless loop).
    ChopSampler offline;
    offline.prepare (sr);
    juce::AudioBuffer<float> scratch (2, 512);
    const double bps = bpm / 60.0 / sr;
    const float pitch = apvts.getRawParameterValue ("pitch")->load();

    for (int pass = 0; pass < 2; ++pass)
        for (int done = 0; done < total;)
        {
            const int n = juce::jmin (512, total - done);
            scratch.clear();
            offline.processPattern (scratch, 0, n, (pass * total + done) * bps, bps, *state, pitch);
            if (pass == 1)
                for (int ch = 0; ch < 2; ++ch)
                    out.copyFrom (ch, done, scratch, ch, 0, n);
            done += n;
        }

    const float tone = apvts.getRawParameterValue ("tone")->load();
    if (tone < 19999.0f)
    {
        juce::IIRFilter f[2];
        for (int ch = 0; ch < 2; ++ch)
        {
            f[ch].setCoefficients (juce::IIRCoefficients::makeLowPass (sr, juce::jmin ((double) tone, sr * 0.45), 0.8));
            f[ch].processSamples (out.getWritePointer (ch), total);
        }
    }

    const float mag = out.getMagnitude (0, total);
    if (mag > 0.98f)
        out.applyGain (0.98f / mag);
}

juce::String BasementProcessor::suggestedFileName() const
{
    auto state = uiState;
    const auto style = juce::String (basement::getChopStyle ((basement::ChopStyle) juce::roundToInt (apvts.getRawParameterValue ("style")->load())).name);
    const auto name = state->sample != nullptr ? state->sample->name.retainCharacters ("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-").substring (0, 24) : juce::String ("chop");
    return "Basement_" + name + "_" + style + "_" + juce::String (juce::roundToInt (getTempo())) + "bpm_" + juce::String (seed.load()) + ".wav";
}

bool BasementProcessor::renderToFile (const juce::File& file) const
{
    juce::AudioBuffer<float> audio;
    const double sr = 44100.0;
    renderLoop (audio, sr, getTempo());

    file.deleteFile();
    juce::WavAudioFormat wav;
    auto stream = file.createOutputStream();
    if (stream == nullptr)
        return false;

    std::unique_ptr<juce::AudioFormatWriter> writer (wav.createWriterFor (stream.get(), sr, 2, 24, {}, 0));
    if (writer == nullptr)
        return false;
    stream.release();
    return writer->writeFromAudioSampleBuffer (audio, 0, audio.getNumSamples());
}

juce::File BasementProcessor::renderToTempFile() const
{
    auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("Basement");
    dir.createDirectory();
    auto file = dir.getChildFile (suggestedFileName());
    renderToFile (file);
    return file;
}

//==============================================================================
void BasementProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("seed", (int) seed.load(), nullptr);
    state.setProperty ("samplePath", sample != nullptr ? sample->file.getFullPathName() : juce::String(), nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void BasementProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (! xml->hasTagName (apvts.state.getType()))
            return;

        auto state = juce::ValueTree::fromXml (*xml);
        seed = (uint32_t) juce::jmax (1, (int) state.getProperty ("seed", 1));
        apvts.replaceState (state);

        const auto path = state.getProperty ("samplePath").toString();
        if (juce::File::isAbsolutePath (path) && juce::File (path).existsAsFile())
        {
            juce::String error;
            if (auto s = loadSampleFile (juce::File (path), error))
                sample = s;
        }

        triggerAsyncUpdate();
    }
}

juce::AudioProcessorEditor* BasementProcessor::createEditor() { return new BasementEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BasementProcessor(); }
