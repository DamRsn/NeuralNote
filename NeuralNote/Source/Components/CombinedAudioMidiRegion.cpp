//
// Created by Damien Ronssin on 11.03.23.
//

#include "CombinedAudioMidiRegion.h"

CombinedAudioMidiRegion::CombinedAudioMidiRegion(NeuralNoteAudioProcessor* processor, Keyboard& keyboard)
    : mProcessor(processor)
    , mVBlankAttachment(this, [this]() { _onVBlankCallback(); })
    , mSupportedAudioFileExtensions(AudioUtils::getSupportedAudioFileExtensions())
    , mAudioRegion(processor, mBaseNumPixelsPerSecond)
    , mPianoRoll(processor, keyboard, mBaseNumPixelsPerSecond)
{
    mProcessor->addListenerToStateValueTree(this);
    addAndMakeVisible(mAudioRegion);
    addAndMakeVisible(mPianoRoll);
    mProcessor->getSourceAudioManager()->getAudioThumbnail()->addChangeListener(this);
    _setZoomLevel(mProcessor->getValueTree().getProperty(NnId::ZoomLevelId, 1.0));
}

CombinedAudioMidiRegion::~CombinedAudioMidiRegion()
{
    mProcessor->removeListenerFromStateValueTree(this);
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

void CombinedAudioMidiRegion::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel)
{
    if (event.mods.isCommandDown()) {
        auto time_start_view = mViewportPtr->getViewPositionX() / (mBaseNumPixelsPerSecond * mZoomLevel);
        _setZoomLevel(mZoomLevel + wheel.deltaY);
        mViewportPtr->setViewPosition(roundToInt(time_start_view * mBaseNumPixelsPerSecond * mZoomLevel), 0);
        repaint();
    } else {
        if (!(mShouldCenterView && mProcessor->getState() == PopulatedAudioAndMidiRegions
              && mProcessor->getPlayer()->isPlaying())) {
            Component::mouseWheelMove(event, wheel);
        }
    }
}
void CombinedAudioMidiRegion::mouseMagnify(const MouseEvent& event, float scaleFactor)
{
    auto time_start_view = mViewportPtr->getViewPositionX() / (mBaseNumPixelsPerSecond * mZoomLevel);
    _setZoomLevel(mZoomLevel * scaleFactor);
    mViewportPtr->setViewPosition(roundToInt(time_start_view * mBaseNumPixelsPerSecond * mZoomLevel), 0);
    repaint();
}

void CombinedAudioMidiRegion::filesDropped(const StringArray& files, int x, int y)
{
    ignoreUnused(x);
    ignoreUnused(y);
    mAudioRegion.setIsFileOver(false);

    if (_isFileTypeSupported(files[0])) {
        bool success = mProcessor->getSourceAudioManager()->onFileDrop(files[0]);

        if (success) {
            resizeAccordingToNumSamplesAvailable();
        }

        repaint();
    } else {
        auto supported_format_string = mSupportedAudioFileExtensions.joinIntoString(", ");

        juce::NativeMessageBox::showMessageBoxAsync(
            juce::MessageBoxIconType::NoIcon,
            "Could not load the file.",
            "Check your file format (Accepted formats: " + supported_format_string + ").");
    }
}

void CombinedAudioMidiRegion::fileDragEnter(const StringArray& files, int x, int y)
{
    if (_isFileTypeSupported(files[0])) {
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
    const double duration_available =
        mProcessor->getSourceAudioManager()->getNumSamplesDownAcquired() / BASIC_PITCH_SAMPLE_RATE;

    int thumbnail_width = static_cast<int>(std::round(mZoomLevel * mBaseNumPixelsPerSecond * duration_available));
    mAudioRegion.setThumbnailWidth(thumbnail_width);

    int new_width = std::max(mBaseWidth, thumbnail_width);

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
                                                   mBaseNumPixelsPerSecond,
                                                   mZoomLevel,
                                                   mAudioRegion.getWidth());

        int full_width = mAudioRegion.getWidth();
        int visible_width = mViewportPtr->getWidth();
        int half_visible_width = visible_width / 2;

        auto pixel_offset = static_cast<int>(std::round(std::max(
            0.0,
            std::min(playhead_position, static_cast<double>(full_width)) - static_cast<double>(half_visible_width))));
        auto prev_pixel_offset = mViewportPtr->getViewPositionX();

        if (pixel_offset != prev_pixel_offset)
            mViewportPtr->setViewPosition(pixel_offset, 0);
    }
}

bool CombinedAudioMidiRegion::_isFileTypeSupported(const String& filename) const
{
    return std::find_if(mSupportedAudioFileExtensions.begin(),
                        mSupportedAudioFileExtensions.end(),
                        [filename](const String& extension) { return filename.endsWith(extension); })
           != mSupportedAudioFileExtensions.end();
}

void CombinedAudioMidiRegion::_setZoomLevel(double inZoomLevel)
{
    mZoomLevel = std::clamp(inZoomLevel, mMinZoomLevel, mMaxZoomLevel);
    mPianoRoll.setZoomLevel(mZoomLevel);
    mAudioRegion.setZoomLevel(mZoomLevel);
    mProcessor->getValueTree().setPropertyExcludingListener(this, NnId::ZoomLevelId, mZoomLevel, nullptr);
    resizeAccordingToNumSamplesAvailable();
}

void CombinedAudioMidiRegion::valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged,
                                                       const Identifier& property)
{
    if (property == NnId::ZoomLevelId) {
        auto time_start_view = mViewportPtr->getViewPositionX() / (mBaseNumPixelsPerSecond * mZoomLevel);
        _setZoomLevel(treeWhosePropertyHasChanged.getProperty(property));
        mViewportPtr->setViewPosition(roundToInt(time_start_view * mBaseNumPixelsPerSecond * mZoomLevel), 0);
    }
}
