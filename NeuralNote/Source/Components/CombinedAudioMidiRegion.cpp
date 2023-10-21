//
// Created by Damien Ronssin on 11.03.23.
//

#include "CombinedAudioMidiRegion.h"

CombinedAudioMidiRegion::CombinedAudioMidiRegion(NeuralNoteAudioProcessor* processor, Keyboard& keyboard)
    : mProcessor(processor)
    , mAudioRegion(processor, mNumPixelsPerSecond)
    , mPianoRoll(processor, keyboard, mNumPixelsPerSecond)
    , mVBlankAttachment(this, [this]() { _onVBlankCallback(); })
{
    addAndMakeVisible(mAudioRegion);
    addAndMakeVisible(mPianoRoll);
    mProcessor->getSourceAudioManager()->getAudioThumbnail()->addChangeListener(this);
}

CombinedAudioMidiRegion::~CombinedAudioMidiRegion()
{
    mProcessor->getSourceAudioManager()->getAudioThumbnail()->removeChangeListener(this);
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
    return mProcessor->getState() == EmptyAudioAndMidiRegions || mProcessor->getState() == PopulatedAudioAndMidiRegions;
}

void CombinedAudioMidiRegion::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) {
    if (event.mods.isCommandDown()) {
		std::cout << "test" << std::endl;
		mZoomLevel += wheel.deltaY;
        mZoomLevel = std::min(mZoomLevel, mMaxZoomLevel);
        mZoomLevel = std::max(mZoomLevel, mMinZoomLevel);
        mPianoRoll.mZoomLevel = mZoomLevel;
		resizeAccordingToNumSamplesAvailable();
        mAudioRegion.repaint();
        std::cout << mPianoRoll.getWidth() << std::endl;
        mPianoRoll.repaint();
    }
}

void CombinedAudioMidiRegion::filesDropped(const StringArray& files, int x, int y)
{
    ignoreUnused(x);
    ignoreUnused(y);
    mAudioRegion.setIsFileOver(false);

    if (files[0].endsWith(".wav") || files[0].endsWith(".aiff") || files[0].endsWith(".flac")
        || files[0].endsWith(".mp3") || files[0].endsWith(".ogg")) {
        bool success = mProcessor->getSourceAudioManager()->onFileDrop(files[0]);

        if (success) {
            resizeAccordingToNumSamplesAvailable();
        }

        repaint();
    } else {
        juce::NativeMessageBox::showMessageBoxAsync(
            juce::MessageBoxIconType::NoIcon,
            "Could not load the file.",
            "Check your file format (Accepted formats: .wav, .aiff, .flac, .mp3, .ogg).");
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
    int num_samples_available = mProcessor->getSourceAudioManager()->getNumSamplesDownAcquired();

    int thumbnail_width =
        static_cast<int>(std::round((num_samples_available * mNumPixelsPerSecond) / BASIC_PITCH_SAMPLE_RATE));

    int new_width = std::max(mBaseWidth, thumbnail_width);

    mAudioRegion.setThumbnailWidth(mZoomLevel*thumbnail_width);
    mAudioRegion.setSize(mZoomLevel*thumbnail_width, mAudioRegion.getHeight());

    mAudioRegion.setSize(mZoomLevel*new_width, mAudioRegion.getHeight());
    mPianoRoll.setSize(mZoomLevel*new_width, mPianoRoll.getHeight());

    if (new_width != getWidth()) {
        setSize(new_width, getHeight());
    }
}

void CombinedAudioMidiRegion::setViewportPtr(juce::Viewport* inViewportPtr)
{
    mViewportPtr = inViewportPtr;
}

void CombinedAudioMidiRegion::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == mProcessor->getSourceAudioManager()->getAudioThumbnail()) {
        resizeAccordingToNumSamplesAvailable();

        if (mProcessor->getState() == Recording) {
            if (mViewportPtr)
                mViewportPtr->setViewPositionProportionately(1.0f, 0.0f);
            else
                jassertfalse;
        }

        mAudioRegion.repaint();
    }
}

void CombinedAudioMidiRegion::setCenterView(bool inShouldCenterView)
{
    mShouldCenterView = inShouldCenterView;
}

AudioRegion* CombinedAudioMidiRegion::getAudioRegion()
{
    return &mAudioRegion;
}
PianoRoll* CombinedAudioMidiRegion::getPianoRoll()
{
    return &mPianoRoll;
}

void CombinedAudioMidiRegion::_onVBlankCallback()
{
    if (mShouldCenterView && mProcessor->getState() == PopulatedAudioAndMidiRegions
        && mProcessor->getPlayer()->isPlaying()) {
        _centerViewOnPlayhead();
    }
}

void CombinedAudioMidiRegion::_centerViewOnPlayhead()
{
    if (mProcessor->getState() == PopulatedAudioAndMidiRegions) {
        double playhead_position =
            Playhead::computePlayheadPositionPixel(mProcessor->getPlayer()->getPlayheadPositionSeconds(),
                                                   mProcessor->getSourceAudioManager()->getAudioSampleDuration(),
                                                   mNumPixelsPerSecond,
                                                   mAudioRegion.getWidth());

        int full_width = mAudioRegion.getWidth();
        int visible_width = mViewportPtr->getWidth();
        int half_visible_width = visible_width / 2;

        auto pixel_offset = (int) std::round(
            std::max(0.0, std::min(playhead_position, (double) full_width) - (double) half_visible_width));
        auto prev_pixel_offset = mViewportPtr->getViewPositionX();

        if (pixel_offset != prev_pixel_offset)
            mViewportPtr->setViewPosition(pixel_offset, 0);
    }
}
