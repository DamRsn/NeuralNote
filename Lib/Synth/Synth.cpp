////
//// Created by Damien Ronssin on 21.05.23.
////
//
//#include "Synth.h"
//
//Synth::Synth(AudioProcessor* inProcessor)
//    : mProcessor(inProcessor)
//{
//}
//
//void Synth::prepareToPlay(float inSampleRate, int inSamplesPerBlock)
//{
//    ignoreUnused(inSamplesPerBlock);
//    mSampleRate = inSampleRate;
//
//    for (auto& voice: mVoices)
//    {
//        voice.prepareToPlay(inSampleRate);
//    }
//}
//
//void Synth::processBlock(AudioBuffer<float>& inAudioBuffer)
//{
//    for (int i = 0; i < inAudioBuffer.getNumSamples(); i++)
//    {
//        inAudioBuffer.setSample(0, i, processSample());
//    }
//
//    // Copy first channel to eventual other channels
//    for (int ch = 1; ch < inAudioBuffer.getNumChannels(); ch++)
//    {
//        inAudioBuffer.copyFrom(ch, 0, inAudioBuffer, 0, 0, inAudioBuffer.getNumSamples());
//    }
//}
//
//float Synth::processSample()
//{
//    float out = 0.0f;
//    for (auto& voice: mVoices)
//    {
//        if (voice.isActive())
//        {
//            out += voice.getNextSample();
//        }
//    }
//
//    return out;
//}
//
//void Synth::reset()
//{
//    for (auto& voice: mVoices)
//    {
//        voice.reset();
//    }
//}
//
//void Synth::noteOn(int inMidiNote, float inInitBend, float inAmplitude)
//{
//    for (auto& voice: mVoices)
//    {
//        if (!voice.isActive())
//        {
//            voice.noteOn(inMidiNote, inInitBend, inAmplitude);
//            break;
//        }
//    }
//}
//
//void Synth::noteOff(int inMidiNote)
//{
//    for (auto& voice: mVoices)
//    {
//        if (voice.isActive() && voice.getCurrentNote() == inMidiNote)
//        {
//            voice.noteOff();
//            break;
//        }
//    }
//}
//
//void Synth::shutAllVoices()
//{
//    for (auto& voice: mVoices)
//    {
//        if (voice.isActive())
//        {
//            voice.noteOff();
//        }
//    }
//}
