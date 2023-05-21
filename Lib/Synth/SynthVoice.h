//
// Created by Damien Ronssin on 20.05.23.
//

#ifndef SynthVoice_h
#define SynthVoice_h

#include <JuceHeader.h>

#include "NoteUtils.h"

class SynthVoice
{
public:
    SynthVoice() = default;

    ~SynthVoice() = default;

    void prepareToPlay(float inSampleRate);

    float getNextSample();

    void adjustFrequency(float inMidiNote, bool inForce);

    void reset();

    void noteOn(int inMidiNote, float inInitBend, float inAmplitude);

    void noteOff();

    bool isActive() const;

    float getFrequency() const;

    int getCurrentNote() const;

private:
    juce::ADSR mADSR;
    juce::ADSR::Parameters mADSRParameters = {0.01f, 0.1f, 0.9f, 0.1};
    juce::dsp::Oscillator<float> mOsc =
        juce::dsp::Oscillator<float>([](float inPhase) { return std::sin(inPhase); }, 0);

    int mCurrentMidiNote = 0;
    float mAmplitude = 1.0f;
    float mSampleRate = 44100.0f;
};

#endif // SynthVoice_h
