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
    std::atomic<bool> mIsPlaying = false;

    std::unique_ptr<SynthController> mSynthController;
    std::unique_ptr<MPESynthesiser> mSynth;
};

#endif // Player_h
