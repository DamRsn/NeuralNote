//
// Created by Damien Ronssin on 17.06.23.
//

#include "Playhead.h"
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
    if (mAudioSampleDuration > 0 && mProcessor->getState() == PopulatedAudioAndMidiRegions) {
        auto playhead_x = static_cast<int>(std::round(computePlayheadPositionPixel(
            mCurrentPlayerPlayheadTime, mAudioSampleDuration, mBaseNumPixelsPerSecond, mZoomLevel, getWidth())));

        g.setColour(juce::Colours::white);
        g.drawVerticalLine(playhead_x, 0, static_cast<float>(getHeight()));

        auto playhead_center_x = static_cast<float>(playhead_x) + 0.5f;
        Path triangle;
        triangle.addTriangle(jmax(0.0f, playhead_center_x - mTriangleSide / 2.0f),
                             0,
                             jmin(playhead_center_x + mTriangleSide / 2.0f, static_cast<float>(getWidth())),
                             0,
                             playhead_center_x,
                             mTriangleHeight);
        g.fillPath(triangle);
    }
}

void Playhead::setPlayheadTime(double inNewTime)
{
    mProcessor->getPlayer()->setPlayheadPositionSeconds(inNewTime);
}

double Playhead::computePlayheadPositionPixel(double inPlayheadPositionSeconds,
                                              double inSampleDuration,
                                              double inBaseNumPixelPerSecond,
                                              double inZoomLevel,
                                              int inWidth)
{
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

    if (mCurrentPlayerPlayheadTime != playhead_time || sample_duration != mAudioSampleDuration) {
        mCurrentPlayerPlayheadTime = playhead_time;
        mAudioSampleDuration = sample_duration;
        repaint();
    }
}
