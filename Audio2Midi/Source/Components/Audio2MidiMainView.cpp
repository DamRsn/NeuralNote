//
// Created by Damien Ronssin on 06.03.23.
//

#include "Audio2MidiMainView.h"

Audio2MidiMainView::Audio2MidiMainView(Audio2MidiAudioProcessor& processor)
    : mProcessor(processor)
    , mVisualizationPanel(processor)
    , mTranscriptionOptions(processor)
    , mNoteOptions(processor)
    , mQuantizePanel(processor)
{
    getLookAndFeel().setDefaultSansSerifTypeface(MONTSERRAT_BOLD);

    mRecordButton = std::make_unique<TextButton>("RecordButton");
    mRecordButton->setButtonText("RECORD");
    mRecordButton->setClickingTogglesState(true);
    mRecordButton->setColour(TextButton::ColourIds::buttonOnColourId, juce::Colours::red);
    mRecordButton->setColour(TextButton::ColourIds::buttonColourId,
                             juce::Colours::whitesmoke);
    mRecordButton->setColour(TextButton::ColourIds::textColourOffId,
                             juce::Colours::black);

    mRecordButton->onClick = [this]()
    {
        bool is_on = mRecordButton->getToggleState();

        // Recording started
        if (is_on)
        {
            mVisualizationPanel.startTimerHzAudioThumbnail(10);
            mProcessor.setStateToRecording();
        }
        else
        {
            // Recording has ended, set processor state to processing
            mProcessor.setStateToProcessing();
            mVisualizationPanel.stopTimerAudioThumbnail();
        }

        updateEnablements();
    };

    addAndMakeVisible(*mRecordButton);

    mClearButton = std::make_unique<TextButton>("ClearButton");
    mClearButton->setButtonText("CLEAR");
    mClearButton->setClickingTogglesState(false);
    mClearButton->setColour(TextButton::ColourIds::buttonOnColourId,
                            juce::Colours::black);
    mClearButton->setColour(TextButton::ColourIds::buttonColourId,
                            juce::Colours::whitesmoke);
    mClearButton->setColour(TextButton::ColourIds::textColourOffId, juce::Colours::black);
    mClearButton->onClick = [this]()
    {
        mProcessor.clear();
        mVisualizationPanel.clear();
        updateEnablements();
    };

    addAndMakeVisible(*mClearButton);

    addAndMakeVisible(mVisualizationPanel);
    addAndMakeVisible(mTranscriptionOptions);
    addAndMakeVisible(mNoteOptions);
    addAndMakeVisible(mQuantizePanel);

    startTimerHz(30);

    updateEnablements();
}

void Audio2MidiMainView::resized()
{
    mRecordButton->setBounds(480, 20, 140, 50);
    mClearButton->setBounds(640, 20, 140, 50);

    mVisualizationPanel.setBounds(300, 100, 680, 530);
    mTranscriptionOptions.setBounds(22, 136, 266, 220);
    mNoteOptions.setTopLeftPosition(22, 374);
    mQuantizePanel.setTopLeftPosition(25, 532);
}

void Audio2MidiMainView::paint(Graphics& g)
{
    g.setColour(juce::Colours::lightcyan);
    g.fillAll();

    g.setColour(juce::Colours::black);

    g.drawText("State: " + std::to_string(static_cast<int>(mProcessor.getState())),
               juce::Rectangle<int>(200, 20, 100, 20),
               juce::Justification::centred);
}

void Audio2MidiMainView::timerCallback()
{
    auto processor_state = mProcessor.getState();
    if (mRecordButton->getToggleState() && processor_state != Recording)
    {
        mRecordButton->setToggleState(false, juce::sendNotification);
        mVisualizationPanel.stopTimerAudioThumbnail();
        updateEnablements();
    }

    if (mPrevState != processor_state)
    {
        mPrevState = processor_state;
        updateEnablements();
    }
}

void Audio2MidiMainView::repaintPianoRoll()
{
    mVisualizationPanel.repaintPianoRoll();
}

void Audio2MidiMainView::updateEnablements()
{
    auto current_state = mProcessor.getState();
    mPrevState = current_state;

    if (current_state == EmptyAudioAndMidiRegions)
    {
        mRecordButton->setEnabled(true);
        mClearButton->setEnabled(false);
        mTranscriptionOptions.setEnabled(false);
        mNoteOptions.setEnabled(false);
        mQuantizePanel.setEnabled(false);
    }
    else if (current_state == Recording)
    {
        mRecordButton->setEnabled(true);
        mClearButton->setEnabled(false);
        mTranscriptionOptions.setEnabled(false);
        mNoteOptions.setEnabled(false);
        mQuantizePanel.setEnabled(false);
    }
    else if (current_state == Processing)
    {
        mRecordButton->setEnabled(false);
        mClearButton->setEnabled(false);
        mTranscriptionOptions.setEnabled(false);
        mNoteOptions.setEnabled(false);
        mQuantizePanel.setEnabled(false);
    }
    else if (current_state == PopulatedAudioAndMidiRegions)
    {
        mRecordButton->setEnabled(false);
        mClearButton->setEnabled(true);
        mTranscriptionOptions.setEnabled(true);
        mNoteOptions.setEnabled(true);
        mQuantizePanel.setEnabled(true);
        mVisualizationPanel.setMidiFileDragComponentVisible();
    }

    repaint();
}
