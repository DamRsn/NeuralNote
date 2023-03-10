#include "PluginProcessor.h"
#include "PluginEditor.h"

Audio2MidiAudioProcessor::Audio2MidiAudioProcessor()
    : mThreadPool(1)
{
    mAudioBufferForMIDITranscription.setSize(1, mMaxNumSamplesToConvert);
    mAudioBufferForMIDITranscription.clear();
}

void Audio2MidiAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    mDownSampler.prepareToPlay(sampleRate, samplesPerBlock);
}

void Audio2MidiAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                            juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);

    if (mParameters.recordOn.load())
    {
        if (mState.load() != Recording)
        {
            mState.store(Recording);
            mDownSampler.reset();
        }

        // If we have reached maximum number of samples that can be processed: stop record and launch processing
        int num_new_down_samples =
            mDownSampler.numOutSamplesOnNextProcessBlock(buffer.getNumSamples());

        if (mNumSamplesAcquired + num_new_down_samples >= mMaxNumSamplesToConvert)
        {
            mParameters.recordOn.store(false);
            // TODO: Thread pool add job to run model and get pg

            mState.store(Processing);
        }
        else
        {
            // Down-mix to mono
            buffer.addFrom(0, 0, buffer, 1, 0, buffer.getNumSamples());
            buffer.applyGain(0.5f);

            // Fill buffer with 22050 Hz audio
            int num_samples_written = mDownSampler.processBlock(
                buffer,
                mAudioBufferForMIDITranscription.getWritePointer(0, mNumSamplesAcquired),
                buffer.getNumSamples());

            jassert(num_samples_written <= num_new_down_samples);

            mNumSamplesAcquired += num_samples_written;
        }
    }
    else
    {
        // If we were previously recording but recordOn is no longer true (user clicked record button to stop it).
        if (mState.load() == Recording)
        {
            // Start processing
            mState.store(Processing);
            // TODO: Thread pool add job to run model and get pg
        }
    }

    buffer.clear();
}

juce::AudioProcessorEditor* Audio2MidiAudioProcessor::createEditor()
{
    return new Audio2MidiEditor(*this);
}

void Audio2MidiAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
}

void Audio2MidiAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
}
void Audio2MidiAudioProcessor::clear()
{
    mNumSamplesAcquired = 0;
    mAudioBufferForMIDITranscription.clear();
}

AudioBuffer<float>& Audio2MidiAudioProcessor::getAudioBufferForMidi()
{
    return mAudioBufferForMIDITranscription;
}

int Audio2MidiAudioProcessor::getNumSamplesAcquired()
{
    return mNumSamplesAcquired;
}

void Audio2MidiAudioProcessor::setNumSamplesAcquired(int inNumSamplesAcquired)
{
    mNumSamplesAcquired = inNumSamplesAcquired;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Audio2MidiAudioProcessor();
}
