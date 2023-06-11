//
// Created by Damien Ronssin on 11.06.23.
//

#include "Player.h"

Player::Player(AudioProcessor* inProcessor)
{
    mSynth = std::make_unique<MPESynthesiser>();
    mSynth->setCurrentPlaybackSampleRate(44100);

    for (int i = 0; i < NUM_VOICES_SYNTH; i++)
    {
        mSynth->addVoice(new SynthVoice());
    }

    mSynthController = std::make_unique<SynthController>(inProcessor, mSynth.get());
}

void Player::prepareToPlay(double inSampleRate, int inSamplesPerBlock)
{
    ignoreUnused(inSamplesPerBlock);
    mSynth->setCurrentPlaybackSampleRate(inSampleRate);
    mSynthController->setSampleRate(inSampleRate);
}

void Player::processBlock(AudioBuffer<float>& inAudioBuffer)
{
    if (mIsPlaying.load())
    {
        auto& midi_buffer = mSynthController->generateNextMidiBuffer(inAudioBuffer.getNumSamples());
        mSynth->renderNextBlock(inAudioBuffer, midi_buffer, 0, inAudioBuffer.getNumSamples());
    }
    else
    {
        mSynth->renderNextBlock(inAudioBuffer, {}, 0, inAudioBuffer.getNumSamples());
    }

    for (int ch = 1; ch < inAudioBuffer.getNumChannels(); ch++)
    {
        inAudioBuffer.copyFrom(ch, 0, inAudioBuffer, 0, 0, inAudioBuffer.getNumSamples());
    }
}

void Player::setPlayingState(bool inIsPlaying)
{
    mIsPlaying.store(inIsPlaying);

    if (!inIsPlaying)
        mSynth->turnOffAllVoices(true);
}

void Player::reset()
{
    mSynthController->reset();
    setPlayingState(false);
}

double Player::getPlayheadPositionSeconds() const
{
    return mSynthController->getCurrentTimeSeconds();
}

void Player::setPlayheadPositionSeconds(double inNewPosition)
{
    mSynthController->setNewTimeSeconds(inNewPosition);
}

SynthController* Player::getSynthController()
{
    return mSynthController.get();
}
