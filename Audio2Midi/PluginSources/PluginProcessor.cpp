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

    if (mState.load() == Recording)
    {
        if (!mWasRecording)
        {
            mDownSampler.reset();
            mWasRecording = true;
        }

        // If we have reached maximum number of samples that can be processed: stop record and launch processing
        int num_new_down_samples =
            mDownSampler.numOutSamplesOnNextProcessBlock(buffer.getNumSamples());

        // If we reach the maximum number of sample that can be gathered
        if (mNumSamplesAcquired + num_new_down_samples >= mMaxNumSamplesToConvert)
        {
            mWasRecording = false;
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
        // If we were previously recording but not anymore (user clicked record button to stop it).
        if (mWasRecording)
        {
            mWasRecording = false;
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
    jassert(mState.load() == PopulatedAudioAndMidiRegions);

    mNumSamplesAcquired = 0;
    mAudioBufferForMIDITranscription.clear();

    mBasicPitch.reset();
    mWasRecording = false;
    mState.store(EmptyAudioAndMidiRegions);
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
    jassert(mState.load() == Processing);
    if (mNumSamplesAcquired >= 1 * AUDIO_SAMPLE_RATE)
    {
        mThreadPool.addJob(mJobLambda);
    }
    else
    {
        mState.store(EmptyAudioAndMidiRegions);
    }
}

const std::vector<Notes::Event>& Audio2MidiAudioProcessor::getNoteEventVector() const
{
    return mBasicPitch.getNoteEvents();
}

Audio2MidiAudioProcessor::Parameters* Audio2MidiAudioProcessor::getCustomParameters()
{
    return &mParameters;
}

void Audio2MidiAudioProcessor::_runModel()
{
    mBasicPitch.transribeToMIDI(mAudioBufferForMIDITranscription.getWritePointer(0),
                                mNumSamplesAcquired);
    mState.store(PopulatedAudioAndMidiRegions);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Audio2MidiAudioProcessor();
}
