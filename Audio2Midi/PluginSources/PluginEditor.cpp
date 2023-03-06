#include "PluginProcessor.h"
#include "PluginEditor.h"

Audio2MidiEditor::Audio2MidiEditor(
    Audio2MidiAudioProcessor& p)
    : AudioProcessorEditor(&p)
{
    mMainView = std::make_unique<Audio2MidiMainView>(p);

    addAndMakeVisible(*mMainView);
    setSize(400, 300);
}

void Audio2MidiEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void Audio2MidiEditor::resized()
{
    mMainView->setBounds(getLocalBounds());
}
