//
// Created by Damien Ronssin on 11.03.23.
//

#include "CombinedAudioMidiRegion.h"

CombinedAudioMidiRegion::CombinedAudioMidiRegion(NeuralNoteAudioProcessor& processor, Keyboard& keyboard)
    : mProcessor(processor)
    , mAudioRegion(processor)
    , mPianoRoll(processor, keyboard, mNumPixelsPerSecond)
{
    addAndMakeVisible(mAudioRegion);
    addAndMakeVisible(mPianoRoll);

    mPlayPauseButton.setButtonText("Play");
    mPlayPauseButton.setClickingTogglesState(true);
    mPlayPauseButton.setToggleState(false, NotificationType::dontSendNotification);
    mPlayPauseButton.onClick = [this]()
    {
        mProcessor.getPlayer()->setPlayingState(mPlayPauseButton.getToggleState());
        repaint();
    };
    addAndMakeVisible(mPlayPauseButton);

    mResetButton.setButtonText("Reset");
    mResetButton.setClickingTogglesState(false);
    mResetButton.onClick = [this]()
    {
        mProcessor.getPlayer()->reset();
        mPlayPauseButton.setToggleState(false, juce::sendNotification);
        repaint();
    };

    addAndMakeVisible(mResetButton);
}

void CombinedAudioMidiRegion::resized()
{
    mAudioRegion.setBounds(0, 0, getWidth(), mAudioRegionHeight);
    mPianoRoll.setBounds(0, mPianoRollY, getWidth(), getHeight() - mPianoRollY);

    mPlayPauseButton.setBounds(getWidth() - 60, mPianoRollY + 20, 40, 20);
    mResetButton.setBounds(getWidth() - 110, mPianoRollY + 20, 40, 20);
}

void CombinedAudioMidiRegion::paint(Graphics& g)
{
}

void CombinedAudioMidiRegion::timerCallback()
{
    resizeAccordingToNumSamplesAvailable();
    mAudioRegion.updateThumbnail();

    if (mViewportPtr)
        mViewportPtr->setViewPositionProportionately(1.0f, 0.0f);
    else
        jassertfalse;

    mAudioRegion.repaint();
}

bool CombinedAudioMidiRegion::isInterestedInFileDrag(const StringArray& files)
{
    return mProcessor.getState() == EmptyAudioAndMidiRegions || mProcessor.getState() == PopulatedAudioAndMidiRegions;
}

void CombinedAudioMidiRegion::filesDropped(const StringArray& files, int x, int y)
{
    ignoreUnused(x);
    ignoreUnused(y);
    mAudioRegion.setIsFileOver(false);

    bool success = mAudioRegion.onFileDrop(files[0]);

    if (success)
    {
        resizeAccordingToNumSamplesAvailable();
        mAudioRegion.updateThumbnail();
    }

    repaint();
}

void CombinedAudioMidiRegion::fileDragEnter(const StringArray& files, int x, int y)
{
    mAudioRegion.setIsFileOver(true);
    mAudioRegion.repaint();
}

void CombinedAudioMidiRegion::fileDragExit(const StringArray& files)
{
    mAudioRegion.setIsFileOver(false);
    mAudioRegion.repaint();
}

void CombinedAudioMidiRegion::setBaseWidth(int inWidth)
{
    mBaseWidth = inWidth;
}

void CombinedAudioMidiRegion::repaintPianoRoll()
{
    mPianoRoll.repaint();
}

void CombinedAudioMidiRegion::resizeAccordingToNumSamplesAvailable()
{
    int num_samples_available = mProcessor.getNumSamplesAcquired();

    int thumbnail_width =
        static_cast<int>(std::round((num_samples_available * mNumPixelsPerSecond) / BASIC_PITCH_SAMPLE_RATE));

    int new_width = std::max(mBaseWidth, thumbnail_width);

    mAudioRegion.setThumbnailWidth(thumbnail_width);

    setSize(new_width, getHeight());
}

void CombinedAudioMidiRegion::setViewportPtr(juce::Viewport* inViewportPtr)
{
    mViewportPtr = inViewportPtr;
}

void CombinedAudioMidiRegion::mouseDown(const juce::MouseEvent& e)
{
    if (!(isInterestedInFileDrag({}) && e.originalComponent == &mAudioRegion))
        return;

    mFileChooser = std::make_shared<juce::FileChooser>(
        "Select Audio File", juce::File {}, "*.wav;*.aiff;*.flac", true, false, this);
    mFileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this](const juce::FileChooser& fc)
                              {
                                  if (fc.getResults().isEmpty())
                                      return;

                                  bool success = mAudioRegion.onFileDrop(fc.getResult());

                                  if (success)
                                  {
                                      resizeAccordingToNumSamplesAvailable();
                                      mAudioRegion.updateThumbnail();
                                  }

                                  repaint();
                              });
}
