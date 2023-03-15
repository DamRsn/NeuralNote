//
// Created by Damien Ronssin on 11.03.23.
//

#include "VisualizationPanel.h"

VisualizationPanel::VisualizationPanel(Audio2MidiAudioProcessor& processor)
    : mCombinedAudioMidiRegion(processor, mKeyboard)
    , mMidiFileDrag(processor)
{
    mAudioMidiViewport.setViewedComponent(&mCombinedAudioMidiRegion);
    addAndMakeVisible(mAudioMidiViewport);
    mCombinedAudioMidiRegion.setViewportPtr(&mAudioMidiViewport);

    addAndMakeVisible(mKeyboard);

    mAudioMidiViewport.setScrollBarsShown(false, true, false, false);
    addChildComponent(mMidiFileDrag);
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

    mMidiFileDrag.setBounds(0, mCombinedAudioMidiRegion.mPianoRollY - 13, getWidth(), 13);
}

void VisualizationPanel::paint(Graphics& g)
{
}

void VisualizationPanel::clear()
{
    mCombinedAudioMidiRegion.setSize(getWidth() - KEYBOARD_WIDTH, getHeight());
    mMidiFileDrag.setVisible(false);
}

void VisualizationPanel::startTimerHzAudioThumbnail(int inFreqHz)
{
    mCombinedAudioMidiRegion.startTimerHz(inFreqHz);
}

void VisualizationPanel::stopTimerAudioThumbnail()
{
    mCombinedAudioMidiRegion.stopTimer();
}

void VisualizationPanel::repaintPianoRoll()
{
    mCombinedAudioMidiRegion.repaintPianoRoll();
}

void VisualizationPanel::setMidiFileDragComponentVisible()
{
    mMidiFileDrag.setVisible(true);
}
