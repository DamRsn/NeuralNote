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
    mProcessor.getSourceAudioManager()->getAudioThumbnail()->addChangeListener(this);
}

CombinedAudioMidiRegion::~CombinedAudioMidiRegion()
{
    mProcessor.getSourceAudioManager()->getAudioThumbnail()->removeChangeListener(this);
}

void CombinedAudioMidiRegion::resized()
{
    mAudioRegion.setBounds(0, 0, getWidth(), mAudioRegionHeight);
    mPianoRoll.setBounds(0, mPianoRollY, getWidth(), getHeight() - mPianoRollY);
}

void CombinedAudioMidiRegion::paint(Graphics& g)
{
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

    if (files[0].endsWith(".wav") || files[0].endsWith(".aiff") || files[0].endsWith(".flac")
        || files[0].endsWith(".mp3")) {
        bool success = mProcessor.getSourceAudioManager()->onFileDrop(files[0]);

        if (success) {
            resizeAccordingToNumSamplesAvailable();
        }

        repaint();
    }
}

void CombinedAudioMidiRegion::fileDragEnter(const StringArray& files, int x, int y)
{
    if (files[0].endsWith(".wav") || files[0].endsWith(".aiff") || files[0].endsWith(".flac")
        || files[0].endsWith(".mp3")) {
        mAudioRegion.setIsFileOver(true);
    }

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
    int num_samples_available = mProcessor.getSourceAudioManager()->getNumSamplesDownAcquired();

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
    if (e.originalComponent != &mAudioRegion || mProcessor.getState() != EmptyAudioAndMidiRegions)
        return;

    mFileChooser = std::make_shared<juce::FileChooser>(
        "Select Audio File", juce::File {}, "*.wav;*.aiff;*.flac", true, false, this);

    mFileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this](const juce::FileChooser& fc) {
                                  if (fc.getResults().isEmpty())
                                      return;
                                  filesDropped(StringArray(fc.getResult().getFullPathName()), 1, 1);
                              });
}

void CombinedAudioMidiRegion::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == mProcessor.getSourceAudioManager()->getAudioThumbnail()) {
        if (mProcessor.getState() == Recording) {
            resizeAccordingToNumSamplesAvailable();

            if (mViewportPtr)
                mViewportPtr->setViewPositionProportionately(1.0f, 0.0f);
            else
                jassertfalse;
        }

        mAudioRegion.repaint();
    }
}
