//
// Created by Damien Ronssin on 06.03.23.
//

#include "NeuralNoteMainView.h"

NeuralNoteMainView::NeuralNoteMainView(NeuralNoteAudioProcessor& processor)
    : mProcessor(processor)
    , mVisualizationPanel(processor)
    , mTranscriptionOptions(processor)
    , mNoteOptions(processor)
    , mQuantizePanel(processor)
{
    mRecordButton = std::make_unique<TextButton>("RecordButton");
    mRecordButton->setButtonText("RECORD");
    mRecordButton->setClickingTogglesState(true);
    mRecordButton->setColour(TextButton::ColourIds::buttonColourId, WHITE_TRANSPARENT);
    mRecordButton->setColour(TextButton::ColourIds::textColourOffId, BLACK);
    mRecordButton->setColour(TextButton::ColourIds::buttonOnColourId, BLACK);
    mRecordButton->setColour(TextButton::ColourIds::textColourOnId, RECORD_RED);

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

    mRecordButton->setToggleState(mProcessor.getState() == Recording,
                                  juce::NotificationType::dontSendNotification);

    addAndMakeVisible(*mRecordButton);

    mClearButton = std::make_unique<TextButton>("ClearButton");
    mClearButton->setButtonText("CLEAR");
    mClearButton->setClickingTogglesState(false);
    mClearButton->setColour(TextButton::ColourIds::buttonOnColourId, BLACK);
    mClearButton->setColour(TextButton::ColourIds::buttonColourId, WHITE_TRANSPARENT);
    mClearButton->setColour(TextButton::ColourIds::textColourOffId, BLACK);
    mClearButton->onClick = [this]()
    {
        mProcessor.clear();
        mVisualizationPanel.clear();
        updateEnablements();
    };
    addAndMakeVisible(*mClearButton);

    mMuteButton = std::make_unique<juce::TextButton>("MuteButton");
    mMuteButton->setButtonText("");
    mMuteButton->setClickingTogglesState(true);

    mMuteButton->setColour(juce::TextButton::buttonColourId,
                           juce::Colours::white.withAlpha(0.2f));
    mMuteButton->setColour(juce::TextButton::buttonOnColourId, BLACK);

    mMuteButtonAttachment =
        std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(
            mProcessor.mTree, "MUTE", *mMuteButton);
    addAndMakeVisible(*mMuteButton);

    addAndMakeVisible(mVisualizationPanel);
    addAndMakeVisible(mTranscriptionOptions);
    addAndMakeVisible(mNoteOptions);
    addAndMakeVisible(mQuantizePanel);

    startTimerHz(30);

    updateEnablements();
}

NeuralNoteMainView::~NeuralNoteMainView()
{
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
}

void NeuralNoteMainView::resized()
{
    mRecordButton->setBounds(588, 36, 144, 51);
    mClearButton->setBounds(748, 36, 144, 51);
    mMuteButton->setBounds(943, 38, 24, 24);

    mVisualizationPanel.setBounds(328, 120, 642, 491);
    mTranscriptionOptions.setBounds(29, 120, 274, 190);
    mNoteOptions.setBounds(29, 334, 274, 133);
    mQuantizePanel.setBounds(29, 491, 274, 120);
}

void NeuralNoteMainView::paint(Graphics& g)
{
    auto background_image = juce::ImageCache::getFromMemory(
        BinaryData::background_png, BinaryData::background_pngSize);

    g.drawImage(background_image, getLocalBounds().toFloat());
    g.setFont(LABEL_FONT);
    g.drawFittedText("MUTE OUT",
                     juce::Rectangle<int>(939, 63, 31, 23),
                     juce::Justification::centred,
                     2);
}

void NeuralNoteMainView::timerCallback()
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

    // To avoid getting stuck in processing mode if processBlock is not called anymore and recording is over (can happen in some DAWs).
    if (mProcessor.getState() == Processing && !mProcessor.isJobRunningOrQueued())
    {
        mNumCallbacksStuckInProcessingState += 1;
        if (mNumCallbacksStuckInProcessingState >= 10)
        {
            mProcessor.launchTranscribeJob();
        }
    }
    else
    {
        mNumCallbacksStuckInProcessingState = 0;
    }
}

void NeuralNoteMainView::repaintPianoRoll()
{
    mVisualizationPanel.repaintPianoRoll();
}

void NeuralNoteMainView::updateEnablements()
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
        mVisualizationPanel.startTimerHzAudioThumbnail(10);
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
        mQuantizePanel.setEnabled(mProcessor.canQuantize());
        mVisualizationPanel.stopTimerAudioThumbnail();

        mVisualizationPanel.setMidiFileDragComponentVisible();
    }

    repaint();
}
