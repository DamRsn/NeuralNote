#include "PluginProcessor.h"
#include "PluginEditor.h"

Audio2MidiEditor::Audio2MidiEditor(Audio2MidiAudioProcessor& p)
    : AudioProcessorEditor(&p)
{
    mMainView = std::make_unique<Audio2MidiMainView>(p);

    addAndMakeVisible(*mMainView);
    setSize(1000, 650);
}

void Audio2MidiEditor::paint(juce::Graphics& g)
{
}

void Audio2MidiEditor::resized()
{
    mMainView->setBounds(getLocalBounds());
}
