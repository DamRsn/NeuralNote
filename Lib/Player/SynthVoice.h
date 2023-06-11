//
// Created by Damien Ronssin on 20.05.23.
//

#ifndef SynthVoice_h
#define SynthVoice_h

#include <JuceHeader.h>

#include "NoteUtils.h"

class SynthVoice : public MPESynthesiserVoice
{
public:
    SynthVoice() = default;

    void setCurrentSampleRate(double inSampleRate) override;

    bool isActive() const override;

    int getCurrentMidiNote() const;

    void noteStarted() override;

    void noteStopped(bool allowTailOff) override;

    void notePressureChanged() override;

    void notePitchbendChanged() override;

    void noteTimbreChanged() override;

    void noteKeyStateChanged() override;

    void renderNextBlock(AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

private:
    juce::ADSR mADSR;
    juce::ADSR::Parameters mADSRParameters = {0.01f, 0.1f, 0.9f, 0.1};
    juce::dsp::Oscillator<float> mOsc =
        juce::dsp::Oscillator<float>([](float inPhase) { return std::sin(inPhase); }, 0);

    int mCurrentMidiNote = 0;
    float mAmplitude = 1.0f;
    double mSampleRate = 44100.0;
};

#endif // SynthVoice_h
