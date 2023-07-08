//
// Created by Damien Ronssin on 09.03.23.
//

#include "AudioRegion.h"

AudioRegion::AudioRegion(NeuralNoteAudioProcessor& processor)
    : mProcessor(processor)
{
}

void AudioRegion::resized()
{
}

void AudioRegion::paint(Graphics& g)
{
    auto num_samples_available = mProcessor.getSourceAudioManager()->getNumSamplesDownAcquired();

    auto* thumbnail = mProcessor.getSourceAudioManager()->getAudioThumbnail();

    if (num_samples_available > 0 && thumbnail->isFullyLoaded()) {
        g.setColour(WAVEFORM_BG_COLOR);
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);

        const auto audio_buffer = mProcessor.getSourceAudioManager()->getDownsampledSourceAudioForTranscription();

        auto thumbnail_area = getLocalBounds();
        thumbnail_area.setWidth(mThumbnailWidth);

        g.setColour(WAVEFORM_COLOR);

        thumbnail->drawChannel(g,
                               thumbnail_area,
                               0.0,
                               num_samples_available / BASIC_PITCH_SAMPLE_RATE,
                               0,
                               0.95f / std::max(thumbnail->getApproximatePeak(), 0.1f));
    } else {
        if (mIsFileOver)
            g.setColour(WAVEFORM_BG_COLOR);
        else
            g.setColour(WHITE_TRANSPARENT);

        g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);

        g.setColour(BLACK);
        g.setFont(LARGE_FONT);

        if (mIsFileOver)
            g.drawText("YUMMY!", getLocalBounds(), juce::Justification::centred);
        else
            g.drawText("LOAD OR DROP AN AUDIO FILE", getLocalBounds(), juce::Justification::centred);
    }
}

void AudioRegion::setIsFileOver(bool inIsFileOver)
{
    mIsFileOver = inIsFileOver;
}

void AudioRegion::setThumbnailWidth(int inThumbnailWidth)
{
    mThumbnailWidth = inThumbnailWidth;
}

void AudioRegion::mouseDown(const juce::MouseEvent& e)
{
    if (auto* parent = getParentComponent())
        parent->mouseDown(e);
}
