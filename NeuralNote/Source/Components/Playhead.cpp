//
// Created by Damien Ronssin on 17.06.23.
//

#include "Playhead.h"
Playhead::Playhead(NeuralNoteAudioProcessor* inProcessor, double inNumPixelsPerSecond)
    : mProcessor(inProcessor)
    , mNumPixelsPerSecond(inNumPixelsPerSecond)
{
    setInterceptsMouseClicks(false, false);

    startTimerHz(60);
}

void Playhead::resized()
{
}

void Playhead::paint(Graphics& g)
{
    if (mAudioSampleDuration > 0 && mProcessor->getState() == PopulatedAudioAndMidiRegions) {
        // TODO: Fix this, does not work when the audio does not span the whole width
        float playhead_x = mCurrentPlayerPlayheadTime / mAudioSampleDuration
                           * std::min(mNumPixelsPerSecond * mAudioSampleDuration, (double) getWidth());
        playhead_x = jlimit(0.0f, (float) getWidth(), playhead_x);

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

void Playhead::timerCallback()
{
    auto playhead_time = mProcessor->getPlayer()->getPlayheadPositionSeconds();
    auto sample_duration = mProcessor->getSourceAudioManager()->getAudioSampleDuration();

    if (mCurrentPlayerPlayheadTime != playhead_time || sample_duration != mAudioSampleDuration) {
        mCurrentPlayerPlayheadTime = playhead_time;
        mAudioSampleDuration = sample_duration;
        repaint();
    }
}

void Playhead::setPlayheadTime(double inNewTime)
{
    mProcessor->getPlayer()->setPlayheadPositionSeconds(inNewTime);
}
