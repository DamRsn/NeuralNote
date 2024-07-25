//
// Created by Damien Ronssin on 11.06.23.
//

#include "Player.h"
#include "PluginProcessor.h"

Player::Player(NeuralNoteAudioProcessor* inProcessor)
    : mProcessor(inProcessor)
{
    mProcessor->addListenerToStateValueTree(this);

    mSynth = std::make_unique<MPESynthesiser>();
    mSynth->setCurrentPlaybackSampleRate(44100);

    for (int i = 0; i < NUM_VOICES_SYNTH; i++) {
        mSynth->addVoice(new SynthVoice());
    }

    mSynthController = std::make_unique<SynthController>(inProcessor, mSynth.get());

    setPlayheadPositionSeconds(mProcessor->getValueTree().getProperty(NnId::PlayheadPositionSecId, 0.0));
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
    auto old_audio_gain = mGainSourceAudio;
    auto old_synth_gain = mGainSynth;

    int playhead_index = static_cast<int>(std::round(mPlayheadTime * mSampleRate));

    float audio_gain_db = mProcessor->getParameterValue(ParameterHelpers::AudioPlayerGainId);
    float synth_gain_db = mProcessor->getParameterValue(ParameterHelpers::MidiPlayerGainId);

    _setGains(audio_gain_db, synth_gain_db);

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

    mInternalBuffer.applyGainRamp(0, 0, inAudioBuffer.getNumSamples(), old_synth_gain, mGainSynth);

    for (int ch = 1; ch < num_out_channels; ch++) {
        mInternalBuffer.copyFrom(ch, 0, mInternalBuffer, 0, 0, inAudioBuffer.getNumSamples());
    }

    if (is_playing && mProcessor->getState() == PopulatedAudioAndMidiRegions) {
        const auto& source_buffer = mProcessor->getSourceAudioManager()->getSourceAudioForPlayback();
        int num_samples = std::min(inAudioBuffer.getNumSamples(), source_buffer.getNumSamples() - playhead_index);

        int num_source_channel = source_buffer.getNumChannels();

        for (int ch = 0; ch < num_out_channels; ch++) {
            int source_channel = std::min(ch, num_source_channel - 1);
            mInternalBuffer.addFromWithRamp(ch,
                                            0,
                                            source_buffer.getReadPointer(source_channel) + playhead_index,
                                            num_samples,
                                            old_audio_gain,
                                            mGainSourceAudio);
        }

        playhead_index += num_samples;

        if (playhead_index >= source_buffer.getNumSamples()) {
            playhead_index = 0;
        }

        mPlayheadTime = static_cast<double>(playhead_index) / mSampleRate;
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
    mPlayheadTime = 0;
}

double Player::getPlayheadPositionSeconds() const
{
    return mSynthController->getCurrentTimeSeconds();
}

void Player::setPlayheadPositionSeconds(double inNewPosition)
{
    if (inNewPosition >= 0 && inNewPosition < mProcessor->getSourceAudioManager()->getAudioSampleDuration()) {
        mSynthController->setNewTimeSeconds(inNewPosition);
        mPlayheadTime = inNewPosition;
    }
}

SynthController* Player::getSynthController()
{
    return mSynthController.get();
}

void Player::saveStateToValueTree()
{
    mProcessor->getValueTree().setPropertyExcludingListener(this, NnId::PlayheadPositionSecId, mPlayheadTime, nullptr);
}

void Player::valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property)
{
    if (property == NnId::PlayheadPositionSecId) {
        double new_position = treeWhosePropertyHasChanged.getProperty(property);

        if (mProcessor->getState() != EmptyAudioAndMidiRegions) {
            new_position = std::clamp(new_position, 0.0, mProcessor->getSourceAudioManager()->getAudioSampleDuration());
        } else {
            new_position = 0.0;
        }

        setPlayheadPositionSeconds(new_position);
    }
}

void Player::_setGains(float inGainAudioSourceDB, float inGainSynthDB)
{
    mGainSourceAudio = Decibels::decibelsToGain(inGainAudioSourceDB, -36.0f);
    mGainSynth = Decibels::decibelsToGain(inGainSynthDB, -36.0f);
}
