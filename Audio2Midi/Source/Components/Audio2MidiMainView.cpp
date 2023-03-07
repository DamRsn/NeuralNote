//
// Created by Damien Ronssin on 06.03.23.
//

#include "Audio2MidiMainView.h"

Audio2MidiMainView::Audio2MidiMainView(Audio2MidiAudioProcessor& processor)
    : mProcessor(processor)
{
    mRecordButton = std::make_unique<TextButton>("Record");
    mRecordButton->setButtonText("Record");
    mRecordButton->setClickingTogglesState(true);
    mRecordButton->setColour(TextButton::ColourIds::buttonOnColourId, juce::Colours::red);
    mRecordButton->setColour(TextButton::ColourIds::buttonColourId,
                             juce::Colours::whitesmoke);
    mRecordButton->setColour(TextButton::ColourIds::textColourOffId,
                             juce::Colours::black);

    mRecordButton->onClick = [this]()
    {
        mProcessor.mParameters.recordOn.store(mRecordButton->getToggleState());
        updateEnablement();
    };

    addAndMakeVisible(*mRecordButton);

    mClearButton = std::make_unique<TextButton>("Clear");
    mClearButton->setButtonText("Clear");
    mClearButton->setClickingTogglesState(false);
    mClearButton->setColour(TextButton::ColourIds::buttonOnColourId,
                            juce::Colours::black);
    mClearButton->setColour(TextButton::ColourIds::buttonColourId,
                            juce::Colours::whitesmoke);
    mClearButton->setColour(TextButton::ColourIds::textColourOffId, juce::Colours::black);
    addAndMakeVisible(*mClearButton);

    mModelConfidenceThresholdSlider =
        std::make_unique<RotarySlider>("Model Confidence", 0.05, 0.95, 0.01, true);
    addAndMakeVisible(*mModelConfidenceThresholdSlider);

    mNoteSegmentationSlider =
        std::make_unique<RotarySlider>("Note Segmentation", 0.05, 0.95, 0.01, true);
    addAndMakeVisible(*mNoteSegmentationSlider);

    mMinNoteDuration =
        std::make_unique<RotarySlider>("Min Note Duration", 3, 50, 1, true);
    addAndMakeVisible(*mMinNoteDuration);

    startTimerHz(30);
}

Audio2MidiMainView::~Audio2MidiMainView()
{
}

void Audio2MidiMainView::resized()
{
    mRecordButton->setBounds(300, 30, 70, 70);
    mClearButton->setBounds(300, 130, 70, 70);

    int knob_size = 100;
    mModelConfidenceThresholdSlider->setBounds(20, 20, knob_size, knob_size);
    mNoteSegmentationSlider->setBounds(20, 150, knob_size, knob_size);
    mMinNoteDuration->setBounds(20, 280, knob_size, knob_size);
}

void Audio2MidiMainView::paint(Graphics& g)
{
    g.setColour(juce::Colours::white);
    g.fillAll();

    g.setColour(juce::Colours::black);

    g.drawText(std::to_string(static_cast<int>(mProcessor.getState())),
               getLocalBounds(),
               juce::Justification::centred);
}

void Audio2MidiMainView::timerCallback()
{
    if (mRecordButton->getToggleState() && !mProcessor.mParameters.recordOn.load())
    {
        mRecordButton->setToggleState(false, juce::NotificationType::sendNotification);
    }

    repaint();
}

void Audio2MidiMainView::sliderValueChanged(juce::Slider* inSliderPtr)
{
}

void Audio2MidiMainView::updateEnablement()
{
    if (mProcessor.mParameters.recordOn.load())
    {
        mClearButton->setEnabled(false);
        mNoteSegmentationSlider->setEnabled(false);
        mMinNoteDuration->setEnabled(false);
        mModelConfidenceThresholdSlider->setEnabled(false);
    }
    else
    {
        mClearButton->setEnabled(true);
        mNoteSegmentationSlider->setEnabled(true);
        mMinNoteDuration->setEnabled(true);
        mModelConfidenceThresholdSlider->setEnabled(true);
    }

    repaint();
}
