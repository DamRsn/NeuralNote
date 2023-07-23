//
// Created by Damien Ronssin on 11.06.23.
//

#ifndef Player_h
#define Player_h

#include <JuceHeader.h>

#include "SynthController.h"
#include "SynthVoice.h"

class NeuralNoteAudioProcessor;

class Player
{
public:
    explicit Player(NeuralNoteAudioProcessor* inProcessor);

    void prepareToPlay(double inSampleRate, int inSamplesPerBlock);

    void processBlock(AudioBuffer<float>& inAudioBuffer);

    bool isPlaying() const;

    void setPlayingState(bool inIsPlaying);

    void reset();

    void setPlayheadPositionSeconds(double inNewPosition);

    double getPlayheadPositionSeconds() const;

    SynthController* getSynthController();

    static constexpr int NUM_VOICES_SYNTH = 16;

private:
    void _setGains(float inGainAudioSourceDB, float inGainSynthDB);

    std::atomic<bool> mIsPlaying = false;
    NeuralNoteAudioProcessor* mProcessor;

    std::unique_ptr<SynthController> mSynthController;
    std::unique_ptr<MPESynthesiser> mSynth;

    AudioBuffer<float> mInternalBuffer;

    int mPlayheadIndex = 0;
    double mSampleRate = 44100;

    float mGainSourceAudio = 0;
    float mGainSynth = 0;
};

#endif // Player_h
