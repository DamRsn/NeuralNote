//
// Created by Damien Ronssin on 19.06.23.
//

#include <memory>

#include "SourceAudioManager.h"

SourceAudioManager::SourceAudioManager()
{
}

void SourceAudioManager::prepareToPlay(double inSampleRate, int inSamplesPerBlock)
{
    mSampleRate = inSampleRate;
    mInternalMonoBuffer.setSize(1, inSamplesPerBlock);
    mDownSampler.prepareToPlay(inSampleRate, inSamplesPerBlock);

    // TODO: if file is already loaded -> resample it at new sample rate if needed for playback.
    // TODO: Stop writers and everything if currently recording
}

void SourceAudioManager::processBlock(const AudioBuffer<float>& inBuffer)
{
    if (mIsRecording) {
        ScopedLock sl(mWriterLock);

        // TODO: make sure there are always two channels
        // Write incoming audio to file at native sample rate
        mThreadedWriter->write(inBuffer.getArrayOfReadPointers(), inBuffer.getNumSamples());

        // Downmix to mono
        mInternalMonoBuffer.copyFrom(0, 0, inBuffer, 0, 0, inBuffer.getNumSamples());

        for (int ch = 1; ch < inBuffer.getNumChannels(); ch++) {
            mInternalMonoBuffer.addFrom(0, 0, inBuffer, ch, 0, inBuffer.getNumSamples());
        }

        mInternalMonoBuffer.applyGain(1.0f / (float) inBuffer.getNumChannels());

        // Downsample to basic pitch sample rate
        int num_samples_down = mDownSampler.processBlock(
            mInternalMonoBuffer, mInternalDownsampledBuffer.getWritePointer(0), inBuffer.getNumSamples());

        // Write downsampled audio to file at native sample rate
        mThreadedWriterDown->write(mInternalDownsampledBuffer.getArrayOfReadPointers(), num_samples_down);
    }
}

void SourceAudioManager::startRecording()
{
    // If already recording (should not happen but if it does, simply return)
    if (mIsRecording.load()) {
        jassertfalse;
        return;
    }

    // Init first writer at native sample rate (stereo)
    juce::WavAudioFormat format;
    juce::StringPairArray meta_data_values;

    auto* wav_writer =
        format.createWriterFor(new juce::FileOutputStream(mFile), mSampleRate, 2, 16, meta_data_values, 0);

    mWriterThread.startThread();
    mThreadedWriter = std::make_unique<juce::AudioFormatWriter::ThreadedWriter>(wav_writer, mWriterThread, 32768);

    // Init second writer at basic pitch sample rate (mono)
    juce::WavAudioFormat format_down;
    juce::StringPairArray meta_data_values_down;

    auto* wav_writer_down = format_down.createWriterFor(
        new juce::FileOutputStream(mFileDown), BASIC_PITCH_SAMPLE_RATE, 1, 16, meta_data_values, 0);

    mWriterThreadDown.startThread();
    mThreadedWriterDown =
        std::make_unique<juce::AudioFormatWriter::ThreadedWriter>(wav_writer_down, mWriterThread, 32768);
    mDownSampler.reset();

    mIsRecording.store(true);
}

void SourceAudioManager::stopRecording()
{
    {
        ScopedLock sl(mWriterLock);
        mIsRecording.store(false);
    }

    mThreadedWriter.reset();
    mThreadedWriterDown.reset();
    
    mWriterThread.stopThread(1000);
    mWriterThreadDown.stopThread(1000);
}

bool SourceAudioManager::isRecording() const
{
    return mIsRecording.load();
}
