//
// Created by Damien Ronssin on 21.05.23.
//

#ifndef Synth_h
#define Synth_h

#include <JuceHeader.h>

#include <SynthVoice.h>
#include "Notes.h"

//class Synth : MPESynthesiser
//{
//public:
//    Synth(AudioProcessor* inProcessor);
//
//    ~Synth() = default;
//
//    void prepareToPlay(float inSampleRate, int inSamplesPerBlock);
//
//    void processBlock(juce::AudioBuffer<float>& inAudioBuffer);
//
//    float processSample();
//
//    void reset();
//
//    void noteOn(int inMidiNote, float inInitBend, float inAmplitude);
//
//    void noteOff(int inMidiNote);
//
//    void shutAllVoices();
//
//private:
//
//    AudioProcessor* mProcessor;
//
//    static constexpr size_t NUM_VOICES = 32;
//    std::array<SynthVoice, NUM_VOICES> mVoices;
//
//    std::atomic<bool> isPlaying = false;
//    float mSampleRate;
//
//};

#endif // Synth_h
