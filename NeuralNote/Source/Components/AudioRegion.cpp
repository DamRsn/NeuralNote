//
// Created by Damien Ronssin on 09.03.23.
//

#include "AudioRegion.h"
#include "CombinedAudioMidiRegion.h"

AudioRegion::AudioRegion(NeuralNoteAudioProcessor* processor, double inBaseNumPixelsPerSecond)
    : mProcessor(processor)
    , mPlayhead(processor, inBaseNumPixelsPerSecond)
    , mBaseNumPixelsPerSecond(inBaseNumPixelsPerSecond)
{
    addAndMakeVisible(mPlayhead);
}

void AudioRegion::resized()
{
    mPlayhead.setSize(getWidth(), getHeight());
}

void AudioRegion::paint(Graphics& g)
{
    auto num_samples_available = mProcessor->getSourceAudioManager()->getNumSamplesDownAcquired();

    auto* thumbnail = mProcessor->getSourceAudioManager()->getAudioThumbnail();

    if (num_samples_available > 0 && thumbnail->isFullyLoaded()) {
        g.setColour(WAVEFORM_BG_COLOR);
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);

        auto thumbnail_area = getLocalBounds();
        thumbnail_area.setWidth(mThumbnailWidth);

        g.setColour(WAVEFORM_COLOR);

        thumbnail->drawChannel(g,
                               thumbnail_area,
                               0.0,
                               num_samples_available / BASIC_PITCH_SAMPLE_RATE,
                               0,
                               0.95f / std::max(thumbnail->getApproximatePeak(), 0.1f));
    } else if (mProcessor->getState() == Processing) {
        g.setColour(WAVEFORM_BG_COLOR);
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);
    } else {
        if (mIsFileOver)
            g.setColour(WAVEFORM_BG_COLOR);
        else
            g.setColour(WHITE_TRANSPARENT);

        g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);

        g.setColour(BLACK);
        g.setFont(UIDefines::LARGE_FONT());

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
    if (mProcessor->getState() == EmptyAudioAndMidiRegions) {
        mFileChooser = std::make_shared<juce::FileChooser>(
            "Select Audio File", juce::File {}, "*.wav;*.aiff;*.flac;*.mp3;*.ogg", true, false, this);

        mFileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                                  [this](const juce::FileChooser& fc) {
                                      if (fc.getResults().isEmpty())
                                          return;
                                      auto* parent = dynamic_cast<CombinedAudioMidiRegion*>(getParentComponent());
                                      if (parent) {
                                          parent->filesDropped(StringArray(fc.getResult().getFullPathName()), 1, 1);
                                      }
                                  });
    } else if (mProcessor->getState() == PopulatedAudioAndMidiRegions) {
        mPlayhead.setPlayheadTime(_pixelToTime(static_cast<float>(e.x)));
    }
}

void AudioRegion::setZoomLevel(double inZoomLevel)
{
    mZoomLevel = inZoomLevel;
    mPlayhead.setZoomLevel(inZoomLevel);
    repaint();
}

float AudioRegion::_pixelToTime(float inPixel) const
{
    return inPixel / static_cast<float>(mBaseNumPixelsPerSecond * mZoomLevel);
}
