//
// Created by Damien Ronssin on 20.05.23.
//

#include "SynthVoice.h"

//void SynthVoice::prepareToPlay(float inSampleRate)
//{
//    mADSR.setSampleRate(inSampleRate);
//    mADSR.setParameters(mADSRParameters);
//    mOsc.prepare({double(inSampleRate), 1, 1});
//    mSampleRate = inSampleRate;
//
//    reset();
//}

//float SynthVoice::getNextSample()
//{
//    jassert(isActive());
//    return mAmplitude * mADSR.getNextSample() * mOsc.processSample(0.0);
//}

//void SynthVoice::adjustFrequency(float inMidiNote, bool inForce)
//{
//    mOsc.setFrequency(NoteUtils::midiToHz(inMidiNote), inForce);
//}
//
//void SynthVoice::reset()
//{
//    mAmplitude = 1.0f;
//    mADSR.reset();
//    mOsc.reset();
//}
//
//void SynthVoice::noteOn(int inMidiNote, float inInitBend, float inAmplitude)
//{
//    mCurrentMidiNote = inMidiNote;
//    mAmplitude = inAmplitude;
//    mADSR.noteOn();
//    mOsc.setFrequency(NoteUtils::midiToHz(static_cast<float>(inMidiNote) + inInitBend), true);
//}
//
//void SynthVoice::noteOff()
//{
//    mADSR.noteOff();
//}

void SynthVoice::setCurrentSampleRate(double newRate)
{
    MPESynthesiserVoice::setCurrentSampleRate(newRate);
}

bool SynthVoice::isActive() const
{
    return mADSR.isActive();
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
}
