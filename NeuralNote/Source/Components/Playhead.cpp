//
// Created by Damien Ronssin on 17.06.23.
//

#include "Playhead.h"
Playhead::Playhead(NeuralNoteAudioProcessor* inProcessor, double inNumPixelsPerSecond)
    : mProcessor(inProcessor)
    , mNumPixelsPerSecond(inNumPixelsPerSecond)
    , mVBlankAttachment(this, [this]() { _onVBlankCallback(); })
{
    setInterceptsMouseClicks(false, false);
}

void Playhead::resized()
{
}

void Playhead::paint(Graphics& g)
{
    if (mAudioSampleDuration > 0 && mProcessor->getState() == PopulatedAudioAndMidiRegions) {
        auto playhead_x = static_cast<float>(computePlayheadPositionPixel(
            mCurrentPlayerPlayheadTime, mAudioSampleDuration, mNumPixelsPerSecond, getWidth()));

        g.setColour(juce::Colours::white);
        g.drawLine(playhead_x, 0, playhead_x, (float) getHeight(), 1);

        Path triangle;
        triangle.addTriangle(jmax(0.0f, playhead_x - mTriangleSide / 2.0f),
                             0,
                             jmin(playhead_x + mTriangleSide / 2.0f, (float) getWidth()),
                             0,
                             playhead_x,
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
                                              double inNumPixelPerSecond,
                                              int inWidth)
{
    auto playhead_pos = inPlayheadPositionSeconds / inSampleDuration
                        * std::min(inNumPixelPerSecond * inSampleDuration, (double) inWidth);
    return jlimit(0.0, (double) inWidth, playhead_pos);
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
