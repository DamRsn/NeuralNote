//
// Created by Damien Ronssin on 11.06.23.
//

#include "Player.h"
#include "PluginProcessor.h"

Player::Player(NeuralNoteAudioProcessor* inProcessor)
    : mProcessor(inProcessor)
{
    mSynth = std::make_unique<MPESynthesiser>();
    mSynth->setCurrentPlaybackSampleRate(44100);

    for (int i = 0; i < NUM_VOICES_SYNTH; i++) {
        mSynth->addVoice(new SynthVoice());
    }

    mSynthController = std::make_unique<SynthController>(inProcessor, mSynth.get());
}

void Player::prepareToPlay(double inSampleRate, int inSamplesPerBlock)
{
    mSynth->setCurrentPlaybackSampleRate(inSampleRate);
    mSynthController->setSampleRate(inSampleRate);
    mSampleRate = inSampleRate;
    mInternalBuffer.setSize(2, inSamplesPerBlock);
}

void Player::processBlock(AudioBuffer<float>& inAudioBuffer)
{
    _setGains(mProcessor->getCustomParameters()->gainSourceAudioDb.load(),
              mProcessor->getCustomParameters()->gainSynthDb.load());

    bool is_playing = mIsPlaying.load();
    mInternalBuffer.clear();

    int num_out_channels = mProcessor->getTotalNumOutputChannels();
    jassert(num_out_channels > 0 && num_out_channels <= 2);

    if (is_playing) {
        auto& midi_buffer = mSynthController->generateNextMidiBuffer(inAudioBuffer.getNumSamples());
        mSynth->renderNextBlock(mInternalBuffer, midi_buffer, 0, inAudioBuffer.getNumSamples());
    } else {
        mSynth->renderNextBlock(mInternalBuffer, {}, 0, inAudioBuffer.getNumSamples());
    }

    mInternalBuffer.applyGain(0, 0, inAudioBuffer.getNumSamples(), mGainSynth);

    for (int ch = 1; ch < num_out_channels; ch++) {
        mInternalBuffer.copyFrom(ch, 0, mInternalBuffer, 0, 0, inAudioBuffer.getNumSamples());
    }

    if (is_playing && mProcessor->getState() == PopulatedAudioAndMidiRegions) {
        const auto source_buffer = mProcessor->getSourceAudioManager()->getSourceAudioForPlayback();
        int num_samples = std::min(inAudioBuffer.getNumSamples(), source_buffer.getNumSamples() - mPlayheadIndex);

        int num_source_channel = source_buffer.getNumChannels();

        for (int ch = 0; ch < num_out_channels; ch++) {
            int source_channel = std::min(ch, num_source_channel - 1);
            mInternalBuffer.addFrom(
                ch, 0, source_buffer, source_channel, mPlayheadIndex, num_samples, mGainSourceAudio);
        }

        mPlayheadIndex += num_samples;

        if (mPlayheadIndex >= source_buffer.getNumSamples()) {
            mPlayheadIndex = 0;
        }
    }

    for (int ch = 0; ch < num_out_channels; ch++) {
        inAudioBuffer.addFrom(ch, 0, mInternalBuffer, ch, 0, inAudioBuffer.getNumSamples());
    }
}

bool Player::isPlaying() const
{
    return mIsPlaying.load();
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
    mPlayheadIndex = 0;
}

double Player::getPlayheadPositionSeconds() const
{
    return mSynthController->getCurrentTimeSeconds();
}

void Player::setPlayheadPositionSeconds(double inNewPosition)
{
    if (inNewPosition >= 0 && inNewPosition < mProcessor->getSourceAudioManager()->getAudioSampleDuration()) {
        mSynthController->setNewTimeSeconds(inNewPosition);
        mPlayheadIndex = (int) std::round(inNewPosition * mSampleRate);
    }
}

SynthController* Player::getSynthController()
{
    return mSynthController.get();
}

void Player::_setGains(float inGainAudioSourceDB, float inGainSynthDB)
{
    mGainSourceAudio = Decibels::decibelsToGain(inGainAudioSourceDB);
    mGainSynth = Decibels::decibelsToGain(inGainSynthDB);
}
