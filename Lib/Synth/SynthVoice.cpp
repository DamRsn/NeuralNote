//
// Created by Damien Ronssin on 20.05.23.
//

#include "SynthVoice.h"

void SynthVoice::prepareToPlay(float inSampleRate)
{
    mADSR.setSampleRate(inSampleRate);
    mADSR.setParameters(mADSRParameters);
    mOsc.prepare({double(inSampleRate), 1, 1});
    mSampleRate = inSampleRate;

    reset();
}

float SynthVoice::getNextSample()
{
    jassert(isActive());
    return mAmplitude * mADSR.getNextSample() * mOsc.processSample(0.0);
}

void SynthVoice::adjustFrequency(float inMidiNote, bool inForce)
{
    mOsc.setFrequency(NoteUtils::midiToHz(inMidiNote), inForce);
}

void SynthVoice::reset()
{
    mAmplitude = 1.0f;
    mADSR.reset();
    mOsc.reset();
}

void SynthVoice::noteOn(int inMidiNote, float inInitBend, float inAmplitude)
{
    mCurrentMidiNote = inMidiNote;
    mAmplitude = inAmplitude;
    mADSR.noteOn();
    mOsc.setFrequency(NoteUtils::midiToHz(static_cast<float>(inMidiNote) + inInitBend),
                      true);
}

void SynthVoice::noteOff()
{
    mADSR.noteOff();
}

bool SynthVoice::isActive() const
{
    return mADSR.isActive();
}

float SynthVoice::getFrequency() const
{
    return mOsc.getFrequency();
}

int SynthVoice::getCurrentNote() const
{
    return mCurrentMidiNote;
}
