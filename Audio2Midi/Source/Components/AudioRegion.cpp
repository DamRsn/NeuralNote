//
// Created by Damien Ronssin on 09.03.23.
//

#include "AudioRegion.h"
AudioRegion::AudioRegion(Audio2MidiAudioProcessor& processor)
    : mProcessor(processor)
    , mThumbnailCache(1)
    , mThumbnail(
          mSourceSamplesPerThumbnailSample, mThumbnailFormatManager, mThumbnailCache)
{
}

void AudioRegion::resized()
{
}

void AudioRegion::paint(Graphics& g)
{
    if (mIsFileOver)
        g.setColour(juce::Colours::blue);
    else
        g.setColour(juce::Colours::black);

    g.drawRect(getLocalBounds());

    g.setColour(juce::Colours::black.withAlpha(0.4f));

    auto num_samples_available = mProcessor.getNumSamplesAcquired();

    if (num_samples_available > 0 && mThumbnail.isFullyLoaded())
    {
        const auto audio_buffer = mProcessor.getAudioBufferForMidi();

        auto thumbnail_area = getLocalBounds();
        thumbnail_area.setWidth(mThumbnailWidth);

        mThumbnail.drawChannel(
            g,
            thumbnail_area,
            0.0,
            num_samples_available / BASIC_PITCH_SAMPLE_RATE,
            0,
            0.95f
                / std::max(audio_buffer.getMagnitude(0, 0, num_samples_available), 0.1f));
    }
}

void AudioRegion::updateThumbnail()
{
    int num_samples_available = mProcessor.getNumSamplesAcquired();

    if (num_samples_available > 50 * mSourceSamplesPerThumbnailSample)
    {
        mThumbnail.reset(1, BASIC_PITCH_SAMPLE_RATE, num_samples_available);
        mThumbnail.addBlock(
            0, mProcessor.getAudioBufferForMidi(), 0, num_samples_available);

        repaint();
    }
}

void AudioRegion::setIsFileOver(bool inIsFileOver)
{
    mIsFileOver = inIsFileOver;
}

bool AudioRegion::onFileDrop(const juce::File& inFile)
{
    mIsFileOver = false;

    int num_loaded_samples = 0;
    auto success = mFileLoader.loadAudioFile(
        inFile, mProcessor.getAudioBufferForMidi(), num_loaded_samples);

    if (!success)
    {
        num_loaded_samples = 0;
        juce::NativeMessageBox::showMessageBoxAsync(
            juce::MessageBoxIconType::NoIcon,
            "Could not load the audio sample.",
            "Check your file format (Accepted formats: .wav, .aiff, .flac). The maximum accepted duration is 3 minutes.");

        return false;
    }

    mProcessor.setNumSamplesAcquired(num_loaded_samples);
    mProcessor.setSampleAcquisitionMode(AudioSampleAcquisitionMode::FileDrop);

    mProcessor.setStateToProcessing();
    mProcessor.launchTranscribeJob();

    repaint();
    return true;
}

void AudioRegion::setThumbnailWidth(int inThumbnailWidth)
{
    mThumbnailWidth = inThumbnailWidth;
}
