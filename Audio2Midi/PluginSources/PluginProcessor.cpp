#include "PluginProcessor.h"
#include "PluginEditor.h"

Audio2MidiAudioProcessor::Audio2MidiAudioProcessor()
    : mThreadPool(1)
{
    mAudioBufferForMIDITranscription.setSize(1, mMaxNumSamplesToConvert);
    mAudioBufferForMIDITranscription.clear();

    mJobLambda = [this]() { _runModel(); };
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
            launchTranscribeJob();
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
            launchTranscribeJob();
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

int Audio2MidiAudioProcessor::getNumSamplesAcquired() const
{
    return mNumSamplesAcquired;
}

void Audio2MidiAudioProcessor::setNumSamplesAcquired(int inNumSamplesAcquired)
{
    mNumSamplesAcquired = inNumSamplesAcquired;
}

void Audio2MidiAudioProcessor::launchTranscribeJob()
{
    if (mNumSamplesAcquired >= 1 * AUDIO_SAMPLE_RATE)
    {
        mThreadPool.addJob(mJobLambda);
        mState.store(Processing);
    }
    else
    {
        mState.store(EmptyAudioAndMidiRegions);
    }
}

void Audio2MidiAudioProcessor::_runModel()
{
    mNotesEvent = mBasicPitch.transribeToMIDI(
        mAudioBufferForMIDITranscription.getWritePointer(0), mNumSamplesAcquired);
    mState.store(PopulatedAudioAndMidiRegions);

    std::cout << "Processing finished" << mNotesEvent.size() << std::endl;

    // TODO: Notify UI
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Audio2MidiAudioProcessor();
}
