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
    mRecordButton->setColour(TextButton::ColourIds::buttonColourId, juce::Colours::whitesmoke);
    mRecordButton->setColour(TextButton::ColourIds::textColourOffId, juce::Colours::black);

    addAndMakeVisible(*mRecordButton);

    mClearButton = std::make_unique<TextButton>("Clear");
    mClearButton->setButtonText("Clear");
    mClearButton->setClickingTogglesState(false);
    mClearButton->setColour(TextButton::ColourIds::buttonOnColourId, juce::Colours::black);
    mClearButton->setColour(TextButton::ColourIds::buttonColourId, juce::Colours::whitesmoke);
    mClearButton->setColour(TextButton::ColourIds::textColourOffId, juce::Colours::black);
    addAndMakeVisible(*mClearButton);

}

Audio2MidiMainView::~Audio2MidiMainView()
{
}

void Audio2MidiMainView::resized()
{
    mRecordButton->setBounds(300, 30, 70, 70);
    mClearButton->setBounds(300, 130, 70, 70);
}

void Audio2MidiMainView::paint(Graphics& g)
{
    g.setColour(juce::Colours::white);
    g.fillAll();

    g.setColour(juce::Colours::black);

    g.drawText("YAY", getLocalBounds(), juce::Justification::centred);
}
