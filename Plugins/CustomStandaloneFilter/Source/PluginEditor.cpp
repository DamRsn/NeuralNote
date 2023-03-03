#include "PluginEditor.h"
#include "PluginProcessor.h"

CustomStandaloneEditor::CustomStandaloneEditor(CustomStandaloneProcessor& p)
    : AudioProcessorEditor(&p)
{
    setSize(400, 300);
}

void CustomStandaloneEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void CustomStandaloneEditor::resized()
{
}
