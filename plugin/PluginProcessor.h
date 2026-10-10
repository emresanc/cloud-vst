#pragma once

#include "ChopSampler.h"

#include <juce_audio_processors/juce_audio_processors.h>

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
    bool producesMidi() const override { return false; }
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

    bool loadSample (const juce::File& file, juce::String& error);
    void setSample (std::shared_ptr<SampleAudio> sample); // also used by tests
    bool hasSample() const { return uiState != nullptr && uiState->sample != nullptr; }

    void randomiseSeed();
    void nudgeSeed (int delta);
    uint32_t getSeed() const { return seed; }

    std::shared_ptr<const ChopState> getState() const { return uiState; }
    int getStateVersion() const { return stateVersion.load(); }

    double getPlayheadBeat() const { return playheadBeat.load(); }
    int getLastSlice() const { return lastSlice.load(); }
    double getTempo() const;

    void setInternalPlaying (bool shouldPlay);
    bool isInternalPlaying() const { return internalPlaying.load(); }

    // Renders one loop of the current chop pattern at the current tempo.
    juce::File renderToTempFile() const;
    bool renderToFile (const juce::File& file) const;
    juce::String suggestedFileName() const;

    static constexpr int firstSliceNote = 36; // C1 plays slice 1 from MIDI

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void parameterChanged (const juce::String& id, float newValue) override;
    void handleAsyncUpdate() override;
    void rebuild();
    void updateTone (double sr);
    void renderLoop (juce::AudioBuffer<float>& out, double sr, double bpm) const;

    std::shared_ptr<SampleAudio> sample;
    std::atomic<uint32_t> seed { 1 };
    std::shared_ptr<const ChopState> uiState;
    std::atomic<int> stateVersion { 0 };

    // Hand-over from the message thread to the audio thread.
    juce::SpinLock stateLock;
    std::unique_ptr<ChopState> pendingState, playingState;
    bool hasPendingState = false;

    std::atomic<double> playheadBeat { -1.0 }, hostBpm { 0.0 };
    std::atomic<int> lastSlice { -1 };
    std::atomic<bool> internalPlaying { false }, internalRestart { false };
    double internalBeat = 0.0, expectedBeat = 0.0, sampleRate = 44100.0;
    bool wasPlaying = false;

    ChopSampler sampler;
    juce::IIRFilter toneFilter[2];
    float currentTone = -1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BasementProcessor)
};
