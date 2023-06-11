//
// Created by Damien Ronssin on 11.06.23.
//

#ifndef Player_h
#define Player_h

#include <JuceHeader.h>

#include "SynthController.h"
#include "SynthVoice.h"

class Player
{
public:
    explicit Player(AudioProcessor* inProcessor);

    void prepareToPlay(double inSampleRate, int inSamplesPerBlock);

    void processBlock(AudioBuffer<float>& inAudioBuffer);

    void setPlayingState(bool inIsPlaying);

    void reset();

    double getPlayheadPositionSeconds() const;

    SynthController* getSynthController();

private:
    std::atomic<bool> mIsPlaying = false;

    std::unique_ptr<SynthController> mSynthController;
    std::unique_ptr<MPESynthesiser> mSynth;
};

#endif // Player_h
