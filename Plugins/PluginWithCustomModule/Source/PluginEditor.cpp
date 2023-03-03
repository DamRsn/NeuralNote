#include "PluginProcessor.h"
#include "PluginEditor.h"

PluginWithCustomModuleEditor::PluginWithCustomModuleEditor(PluginWithCustomModule& p)
    : juce::AudioProcessorEditor(&p)
{
    setSize(400, 300);
}

void PluginWithCustomModuleEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}
