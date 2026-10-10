#pragma once

#include "Generator.h"
#include "PreviewSynth.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <bitset>
#include <memory>

class BasementProcessor : public juce::AudioProcessor,
                          private juce::AudioProcessorValueTreeState::Listener,
                          private juce::AsyncUpdater
{
public:
    BasementProcessor();
    ~BasementProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    // Message-thread API used by the editor.
    juce::AudioProcessorValueTreeState apvts;

    void randomiseSeed();
    void nudgeSeed (int delta);
    uint32_t getSeed() const { return seed; }

    std::shared_ptr<const basement::Pattern> getPattern() const { return uiPattern; }
    int getPatternVersion() const { return patternVersion.load(); }
    basement::Settings currentSettings() const;

    // Loop position in beats, or -1 when stopped.
    double getPlayheadBeat() const { return playheadBeat.load(); }
    double getExportTempo() const;

    void setInternalPlaying (bool shouldPlay);
    bool isInternalPlaying() const { return internalPlaying.load(); }
    bool isHostPlaying() const { return hostPlaying.load(); }

    // Writes the current pattern to a temp .mid named after style/key/tempo/seed.
    juce::File writeMidiToTempFile (int layers) const;
    juce::String suggestedFileName (int layers) const;
    juce::MemoryBlock midiData (int layers) const;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void parameterChanged (const juce::String& id, float newValue) override;
    void handleAsyncUpdate() override;
    void regenerate();

    void flushActiveNotes (juce::MidiBuffer& out, int sample);
    void renderPattern (juce::MidiBuffer& out, double startBeat, double beatsPerSample, int numSamples);

    std::atomic<uint32_t> seed { 1 };
    std::shared_ptr<const basement::Pattern> uiPattern;
    std::atomic<int> patternVersion { 0 };

    // Hand-over from the message thread to the audio thread.
    juce::SpinLock patternLock;
    std::unique_ptr<basement::Pattern> pendingPattern, playingPattern;
    bool hasPendingPattern = false;

    std::atomic<double> playheadBeat { -1.0 }, hostBpm { 0.0 };
    std::atomic<bool> internalPlaying { false }, hostPlaying { false }, internalRestart { false };
    double internalBeat = 0.0, expectedBeat = 0.0, sampleRate = 44100.0;
    bool wasPlaying = false;

    struct PendingEvent
    {
        int sample;
        bool on;
        int channel, pitch, velocity;
    };
    std::vector<PendingEvent> scratchEvents;
    std::array<std::bitset<128>, 16> activeNotes;
    juce::MidiBuffer generatedMidi;

    PreviewSynth previewSynth;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BasementProcessor)
};
