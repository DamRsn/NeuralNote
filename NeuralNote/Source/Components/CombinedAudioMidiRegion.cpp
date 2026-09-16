//
// Created by Damien Ronssin on 11.03.23.
//

#include "CombinedAudioMidiRegion.h"

CombinedAudioMidiRegion::CombinedAudioMidiRegion(NeuralNoteAudioProcessor* processor, Keyboard& keyboard)
    : mProcessor(processor)
    , mKeyboard(keyboard)
    , mVBlankAttachment(this, [this]() { _onVBlankCallback(); })
    , mSupportedAudioFileExtensions(AudioUtils::getSupportedAudioFileExtensions())
    , mAudioRegion(processor, mBaseNumPixelsPerSecond)
    , mTimeRuler(processor, mBaseNumPixelsPerSecond)
    , mPianoRoll(processor, keyboard, mBaseNumPixelsPerSecond)
{
    mProcessor->addListenerToStateValueTree(this);
    addAndMakeVisible(mAudioRegion);
    addAndMakeVisible(mTimeRuler);
    addAndMakeVisible(mPianoRoll);
    mProcessor->getSourceAudioManager()->addChangeListener(this);
    _setZoomLevel(mProcessor->getValueTree().getProperty(NnId::ZoomLevelId, 1.0));
}

CombinedAudioMidiRegion::~CombinedAudioMidiRegion()
{
    mProcessor->removeListenerFromStateValueTree(this);
    mProcessor->getSourceAudioManager()->removeChangeListener(this);
}

void CombinedAudioMidiRegion::resized()
{
    mAudioRegion.setBounds(0, 0, getWidth(), mAudioRegionHeight);
    mTimeRuler.setBounds(0, mAudioRegionHeight, getWidth(), mRulerHeight);
    mPianoRoll.setBounds(0, mPianoRollY, getWidth(), getHeight() - mPianoRollY);
}

void CombinedAudioMidiRegion::paint(Graphics& g)
{
    ignoreUnused(g);
}

bool CombinedAudioMidiRegion::isInterestedInFileDrag(const StringArray& files)
{
    ignoreUnused(files);
    const State state = mProcessor->getState();

    // Anything but a run in flight or a recording: dropping replaces whatever is loaded.
    return state == EmptyAudioAndMidiRegions || state == AudioLoaded || state == PopulatedAudioAndMidiRegions;
}

void CombinedAudioMidiRegion::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel)
{
    if (event.mods.isCommandDown()) {
        auto time_start_view = mViewportPtr->getViewPositionX() / (mBaseNumPixelsPerSecond * mZoomLevel);
        _setZoomLevel(mZoomLevel + wheel.deltaY);
        mViewportPtr->setViewPosition(roundToInt(time_start_view * mBaseNumPixelsPerSecond * mZoomLevel), 0);
        repaint();
        return;
    }

    // Over the piano roll, a vertical wheel moves through pitches: it is the only band that can be
    // taller than the space it has. Horizontal still moves through time, here as everywhere else.
    if (event.y >= mPianoRollY && !approximatelyEqual(wheel.deltaY, 0.0f)) {
        mKeyboard.scrollByWheel(event.getEventRelativeTo(&mKeyboard), wheel);
        return;
    }

    // Everything else falls through to the viewport, unless the view is already following the
    // playhead -- where a scroll would be undone on the next frame.
    if (!(mShouldCenterView && mProcessor->canPlay() && mProcessor->getPlayer()->isPlaying())) {
        Component::mouseWheelMove(event, wheel);
    }
}
void CombinedAudioMidiRegion::mouseMagnify(const MouseEvent& event, float scaleFactor)
{
    ignoreUnused(event);
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
            refreshForAudioLength();
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
    ignoreUnused(x, y);
    if (_isFileTypeSupported(files[0])) {
        mAudioRegion.setIsFileOver(true);
    }

    mAudioRegion.repaint();
}

void CombinedAudioMidiRegion::fileDragExit(const StringArray& files)
{
    ignoreUnused(files);
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

void CombinedAudioMidiRegion::updateEnablements()
{
    mAudioRegion.updateEnablements();
    mPianoRoll.updateEnablements();
}

void CombinedAudioMidiRegion::resizeAccordingToNumSamplesAvailable()
{
    const double duration_available =
        mProcessor->getSourceAudioManager()->getNumSamplesDownAcquired() / TRANSCRIPTION_SAMPLE_RATE;

    int waveform_width = static_cast<int>(std::round(mZoomLevel * mBaseNumPixelsPerSecond * duration_available));
    int new_width = std::max(mBaseWidth, waveform_width);

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
    if (source == mProcessor->getSourceAudioManager()) {
        refreshForAudioLength();

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
    if (mShouldCenterView && mProcessor->canPlay() && mProcessor->getPlayer()->isPlaying()) {
        _centerViewOnPlayhead();
    }
}

void CombinedAudioMidiRegion::_centerViewOnPlayhead()
{
    if (mProcessor->canPlay()) {
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

double CombinedAudioMidiRegion::_minZoomLevel() const
{
    const double duration = mProcessor->getSourceAudioManager()->getAudioSampleDuration();

    if (duration <= 0.0 || mBaseWidth <= 0) {
        return mMinZoomLevel;
    }

    // Never below the floor, and never above the ceiling either: a take shorter than the viewport
    // cannot fill it at any zoom, and an inverted range would make the clamp meaningless.
    const double fits_viewport = mBaseWidth / (mBaseNumPixelsPerSecond * duration);

    return std::clamp(fits_viewport, mMinZoomLevel, mMaxZoomLevel);
}

void CombinedAudioMidiRegion::refreshForAudioLength()
{
    const double allowed = std::clamp(mZoomLevel, _minZoomLevel(), mMaxZoomLevel);

    if (juce::approximatelyEqual(allowed, mZoomLevel)) {
        resizeAccordingToNumSamplesAvailable();
        return;
    }

    // Loading a shorter take can leave the view zoomed out past the end of it.
    _setZoomLevel(allowed);
}

void CombinedAudioMidiRegion::_setZoomLevel(double inZoomLevel)
{
    mZoomLevel = std::clamp(inZoomLevel, _minZoomLevel(), mMaxZoomLevel);
    mPianoRoll.setZoomLevel(mZoomLevel);
    mTimeRuler.setZoomLevel(mZoomLevel);
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
