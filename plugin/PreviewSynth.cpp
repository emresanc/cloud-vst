#include "PreviewSynth.h"
#include "Generator.h"

namespace
{

enum class Timbre
{
    Bell,
    Pad,
    Sub
};

struct PreviewSound : public juce::SynthesiserSound
{
    PreviewSound (Timbre t, int ch) : timbre (t), channel (ch) {}

    bool appliesToNote (int) override { return true; }
    bool appliesToChannel (int midiChannel) override { return midiChannel == channel; }

    const Timbre timbre;
    const int channel;
};

class PreviewVoice : public juce::SynthesiserVoice
{
public:
    explicit PreviewVoice (Timbre t) : timbre (t) {}

    bool canPlaySound (juce::SynthesiserSound* s) override
    {
        auto* sound = dynamic_cast<PreviewSound*> (s);
        return sound != nullptr && sound->timbre == timbre;
    }

    void startNote (int note, float velocity, juce::SynthesiserSound*, int) override
    {
        const double sr = getSampleRate();
        freq = juce::MidiMessage::getMidiNoteInHertz (note);
        phase = phase2 = 0.0;
        level = velocity;
        time = 0.0;
        lp = 0.0f;
        dt = 1.0 / sr;

        juce::ADSR::Parameters p;
        switch (timbre)
        {
            case Timbre::Bell: p = { 0.002f, 1.2f, 0.0f, 0.25f }; break;
            case Timbre::Pad:  p = { 0.08f, 0.6f, 0.7f, 0.5f }; break;
            case Timbre::Sub:  p = { 0.002f, 2.5f, 0.0f, 0.12f }; break;
        }
        env.setSampleRate (sr);
        env.setParameters (p);
        env.noteOn();
    }

    void stopNote (float, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            env.noteOff();
        }
        else
        {
            env.reset();
            clearCurrentNote();
        }
    }

    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

    void renderNextBlock (juce::AudioBuffer<float>& out, int start, int num) override
    {
        if (! isVoiceActive())
            return;

        const double twoPi = juce::MathConstants<double>::twoPi;

        for (int i = 0; i < num; ++i)
        {
            const float e = env.getNextSample();
            float s = 0.0f;

            switch (timbre)
            {
                case Timbre::Bell:
                {
                    // 2-op FM with a decaying index: glassy dark-trap bell.
                    const double index = 2.2 * std::exp (-time * 6.0);
                    s = (float) std::sin (phase + index * std::sin (phase2)) * 0.16f;
                    phase += twoPi * freq * dt;
                    phase2 += twoPi * freq * 3.5 * dt;
                    break;
                }
                case Timbre::Pad:
                {
                    // Two detuned saws through a one-pole low-pass.
                    const double saw1 = 2.0 * (phase / twoPi) - 1.0;
                    const double saw2 = 2.0 * (phase2 / twoPi) - 1.0;
                    const float raw = (float) (saw1 + saw2) * 0.5f;
                    lp += 0.08f * (raw - lp);
                    s = lp * 0.07f;
                    phase = std::fmod (phase + twoPi * freq * 1.003 * dt, twoPi);
                    phase2 = std::fmod (phase2 + twoPi * freq * 0.997 * dt, twoPi);
                    break;
                }
                case Timbre::Sub:
                {
                    // Sine with a quick pitch drop and soft clip, like an 808.
                    const double f = freq * (1.0 + 1.5 * std::exp (-time * 60.0));
                    s = (float) std::tanh (1.8 * std::sin (phase)) * 0.3f;
                    phase += twoPi * f * dt;
                    break;
                }
            }

            phase = std::fmod (phase, twoPi * 64.0);
            phase2 = std::fmod (phase2, twoPi * 64.0);
            time += dt;

            const float v = s * e * level;
            for (int ch = 0; ch < out.getNumChannels(); ++ch)
                out.addSample (ch, start + i, v);

            if (! env.isActive())
            {
                clearCurrentNote();
                break;
            }
        }
    }

private:
    const Timbre timbre;
    juce::ADSR env;
    double freq = 440.0, phase = 0, phase2 = 0, time = 0, dt = 1.0 / 44100.0;
    float level = 1.0f, lp = 0.0f;
};

} // namespace

PreviewSynth::PreviewSynth()
{
    synth.addSound (new PreviewSound (Timbre::Bell, basement::MelodyChannel));
    synth.addSound (new PreviewSound (Timbre::Pad, basement::ChordChannel));
    synth.addSound (new PreviewSound (Timbre::Sub, basement::BassChannel));

    for (int i = 0; i < 8; ++i)
        synth.addVoice (new PreviewVoice (Timbre::Bell));
    for (int i = 0; i < 10; ++i)
        synth.addVoice (new PreviewVoice (Timbre::Pad));
    for (int i = 0; i < 2; ++i)
        synth.addVoice (new PreviewVoice (Timbre::Sub));
}

void PreviewSynth::prepare (double sampleRate)
{
    synth.setCurrentPlaybackSampleRate (sampleRate);
}

void PreviewSynth::render (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi)
{
    synth.renderNextBlock (buffer, midi, 0, buffer.getNumSamples());
}

void PreviewSynth::allNotesOff()
{
    synth.allNotesOff (0, false);
}
