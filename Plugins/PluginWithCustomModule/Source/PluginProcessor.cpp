#include "PluginProcessor.h"
#include "PluginEditor.h"

void PluginWithCustomModule::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)

{
    whiteNoise.process(buffer);
}

juce::AudioProcessorEditor* PluginWithCustomModule::createEditor()
{
    return new PluginWithCustomModuleEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginWithCustomModule();
}
