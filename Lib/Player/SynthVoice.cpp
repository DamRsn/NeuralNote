//
// Created by Damien Ronssin on 20.05.23.
//

#include "SynthVoice.h"

void SynthVoice::setCurrentSampleRate(double inSampleRate)
{
    mSampleRate = inSampleRate;

    MPESynthesiserVoice::setCurrentSampleRate(inSampleRate);
    mADSR.setSampleRate(inSampleRate);
    mADSR.setParameters(mADSRParameters);
    mOsc.prepare({double(inSampleRate), 1, 1});

    mADSR.reset();
    mOsc.reset();
}

bool SynthVoice::isActive() const
{
    return mADSR.isActive();
}

int SynthVoice::getCurrentMidiNote() const
{
    return mCurrentMidiNote;
}

void SynthVoice::noteStarted()
{
    mADSR.reset();
    mOsc.reset();

    auto mpe_note = getCurrentlyPlayingNote();
    mCurrentMidiNote = mpe_note.initialNote;
    mAmplitude = mpe_note.noteOnVelocity.asUnsignedFloat();
    mADSR.noteOn();
    mOsc.setFrequency((float) mpe_note.getFrequencyInHertz(), true);
}

void SynthVoice::noteStopped(bool allowTailOff)
{
    mADSR.noteOff();
}

void SynthVoice::notePressureChanged()
{
}

void SynthVoice::notePitchbendChanged()
{
    mOsc.setFrequency((float) getCurrentlyPlayingNote().getFrequencyInHertz(), false);
}

void SynthVoice::noteTimbreChanged()
{
}

void SynthVoice::noteKeyStateChanged()
{
}

void SynthVoice::renderNextBlock(AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    for (int i = startSample; i < numSamples; i++)
    {
        float value = mAmplitude * mADSR.getNextSample() * mOsc.processSample(0);
        outputBuffer.addSample(0, i, value);

        if (!mADSR.isActive())
        {
            clearCurrentNote();
            return;
        }
    }
}
