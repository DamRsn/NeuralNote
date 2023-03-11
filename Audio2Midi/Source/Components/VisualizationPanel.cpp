//
// Created by Damien Ronssin on 11.03.23.
//

#include "VisualizationPanel.h"
VisualizationPanel::VisualizationPanel(Audio2MidiAudioProcessor& processor)
    : mCombinedAudioMidiRegion(processor, mKeyboard)
{
    mAudioMidiViewport.setViewedComponent(&mCombinedAudioMidiRegion);
    addAndMakeVisible(mKeyboard);
    addAndMakeVisible(mAudioMidiViewport);

    mAudioMidiViewport.setScrollBarsShown(false, true, false, false);
}

void VisualizationPanel::resized()
{
    mKeyboard.setBounds(0,
                        mCombinedAudioMidiRegion.mPianoRollY,
                        KEYBOARD_WIDTH,
                        getHeight() - mCombinedAudioMidiRegion.mPianoRollY);

    mAudioMidiViewport.setBounds(
        KEYBOARD_WIDTH, 0, getWidth() - KEYBOARD_WIDTH, getHeight());

    mCombinedAudioMidiRegion.setBounds(
        KEYBOARD_WIDTH, 0, getWidth() - KEYBOARD_WIDTH, getHeight());

    mCombinedAudioMidiRegion.setBaseWidth(getWidth() - KEYBOARD_WIDTH);
}

void VisualizationPanel::paint(Graphics& g)
{
}

void VisualizationPanel::clear()
{
    mCombinedAudioMidiRegion.setSize(getWidth() - KEYBOARD_WIDTH, getHeight());
}

void VisualizationPanel::startTimerHzAudioThumbnail(int inFreqHz)
{
    mCombinedAudioMidiRegion.startTimerHz(inFreqHz);
}

void VisualizationPanel::stopTimerAudioThumbnail()
{
    mCombinedAudioMidiRegion.stopTimer();
}
