//
// Created by Damien Ronssin on 17.06.23.
//

#include "Playhead.h"

#include "NnLook.h"

Playhead::Playhead(NeuralNoteAudioProcessor* inProcessor, double inNumPixelsPerSecond)
    : mProcessor(inProcessor)
    , mVBlankAttachment(this, [this]() { _onVBlankCallback(); })
    , mBaseNumPixelsPerSecond(inNumPixelsPerSecond)
{
    setInterceptsMouseClicks(false, false);
}

void Playhead::resized()
{
}

void Playhead::paint(Graphics& g)
{
    if (mAudioSampleDuration <= 0 || !mProcessor->canPlay()) {
        return;
    }

    const auto playhead_x = static_cast<int>(std::round(getPlayheadX()));

    g.setColour(nn::colours::textBright);
    g.drawVerticalLine(playhead_x, 0, static_cast<float>(getHeight()));

    if (!mDrawTriangle) {
        return;
    }

    const auto playhead_centre_x = static_cast<float>(playhead_x) + 0.5f;
    Path triangle;
    triangle.addTriangle(jmax(0.0f, playhead_centre_x - mTriangleSide / 2.0f),
                         0,
                         jmin(playhead_centre_x + mTriangleSide / 2.0f, static_cast<float>(getWidth())),
                         0,
                         playhead_centre_x,
                         mTriangleHeight);
    g.fillPath(triangle);
}

void Playhead::setPlayheadTime(double inNewTime)
{
    mProcessor->getPlayer()->setPlayheadPositionSeconds(inNewTime);
}

void Playhead::setDrawTriangle(bool inShouldDraw)
{
    mDrawTriangle = inShouldDraw;
}

double Playhead::getPlayheadX() const
{
    return computePlayheadPositionPixel(
        mCurrentPlayerPlayheadTime, mAudioSampleDuration, mBaseNumPixelsPerSecond, mZoomLevel, getWidth());
}

double Playhead::computePlayheadPositionPixel(double inPlayheadPositionSeconds,
                                              double inSampleDuration,
                                              double inBaseNumPixelPerSecond,
                                              double inZoomLevel,
                                              int inWidth)
{
    if (inSampleDuration <= 0.0) {
        return 0.0;
    }

    auto playhead_pos =
        inPlayheadPositionSeconds / inSampleDuration
        * std::min(inBaseNumPixelPerSecond * inZoomLevel * inSampleDuration, static_cast<double>(inWidth));
    return jlimit(0.0, static_cast<double>(inWidth), playhead_pos);
}

void Playhead::setZoomLevel(double inZoomLevel)
{
    mZoomLevel = inZoomLevel;
}

void Playhead::_onVBlankCallback()
{
    auto playhead_time = mProcessor->getPlayer()->getPlayheadPositionSeconds();
    auto sample_duration = mProcessor->getSourceAudioManager()->getAudioSampleDuration();

    if (juce::exactlyEqual(mCurrentPlayerPlayheadTime, playhead_time)
        && juce::exactlyEqual(sample_duration, mAudioSampleDuration)) {
        return;
    }

    const int previous_x = juce::roundToInt(getPlayheadX());
    const bool was_drawing = mIsDrawing;

    mCurrentPlayerPlayheadTime = playhead_time;
    mAudioSampleDuration = sample_duration;
    mIsDrawing = mAudioSampleDuration > 0 && mProcessor->canPlay();

    // This component is transparent and as wide as the whole timeline, so a full repaint drags
    // everything underneath it -- the waveform and the piano roll -- through a full-width redraw
    // every frame. Only the two slivers the line has left and arrived at have actually changed.
    if (was_drawing != mIsDrawing) {
        repaint();
        return;
    }

    if (!mIsDrawing) {
        return;
    }

    const int new_x = juce::roundToInt(getPlayheadX());

    if (new_x == previous_x) {
        return;
    }

    // Wide enough for the triangle, which is drawn centred on the line and is the widest thing here.
    const int margin = static_cast<int>(mTriangleSide / 2.0f) + 1;
    const int from = std::min(new_x, previous_x) - margin;
    const int to = std::max(new_x, previous_x) + margin;

    repaint(from, 0, to - from, getHeight());
}
