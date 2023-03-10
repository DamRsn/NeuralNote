//
// Created by Damien Ronssin on 09.03.23.
//

#include "AudioRegion.h"
AudioRegion::AudioRegion(Audio2MidiAudioProcessor& processor)
    : mAudioProcessor(processor)
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

    g.setColour(juce::Colours::black);

    auto num_samples_available = mAudioProcessor.getNumSamplesAcquired();

    if (num_samples_available > 0 && mThumbnail.isFullyLoaded())
    {
        const auto audio_buffer = mAudioProcessor.getAudioBufferForMidi();

        mThumbnail.drawChannel(
            g,
            getLocalBounds(),
            0.0,
            num_samples_available / BASIC_PITCH_SAMPLE_RATE,
            0,
            0.95f
                / std::max(audio_buffer.getMagnitude(0, 0, num_samples_available), 0.1f));
    }
}

void AudioRegion::timerCallback()
{
    int num_samples_available = mAudioProcessor.getNumSamplesAcquired();

    if (num_samples_available > 50 * mSourceSamplesPerThumbnailSample)
    {
        mThumbnail.reset(1, BASIC_PITCH_SAMPLE_RATE, num_samples_available);
        mThumbnail.addBlock(
            0, mAudioProcessor.getAudioBufferForMidi(), 0, num_samples_available);

        repaint();
    }
}

bool AudioRegion::isInterestedInFileDrag(const StringArray& files)
{
    return mAudioProcessor.getState() == EmptyAudioAndMidiRegions
           || mAudioProcessor.getState() == PopulatedAudioAndMidiRegions;
}

void AudioRegion::filesDropped(const StringArray& files, int x, int y)
{
    ignoreUnused(x);
    ignoreUnused(y);
    mIsFileOver = false;

    int num_loaded_samples = 0;
    auto success = mFileLoader.loadAudioFile(
        files[0], mAudioProcessor.getAudioBufferForMidi(), num_loaded_samples);

    if (!success)
    {
        num_loaded_samples = 0;
        juce::NativeMessageBox::showMessageBoxAsync(
            juce::MessageBoxIconType::NoIcon,
            "Could not load the audio sample.",
            "Check your file format (Accepted formats: .wav, .aiff, .flac). The maximum accepted duration is 3 minutes.");
    }

    mAudioProcessor.setNumSamplesAcquired(num_loaded_samples);

    mThumbnail.reset(1, BASIC_PITCH_SAMPLE_RATE, num_loaded_samples);
    mThumbnail.addBlock(
        0, mAudioProcessor.getAudioBufferForMidi(), 0, num_loaded_samples);

    mAudioProcessor.launchTranscribeJob();

    repaint();
}

void AudioRegion::fileDragEnter(const StringArray& files, int x, int y)
{
    ignoreUnused(x);
    ignoreUnused(y);
    // Paint something to indicate you can drop here
    mIsFileOver = true;
    repaint();
}

void AudioRegion::fileDragExit(const StringArray& files)
{
    ignoreUnused(files);
    mIsFileOver = false;
    repaint();
}
