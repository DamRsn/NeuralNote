#include "PluginProcessor.h"
#include "PluginEditor.h"

void CustomStandaloneProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& midiMessages)

{
    midiMessages.clear();
    buffer.clear();
}

juce::AudioProcessorEditor* CustomStandaloneProcessor::createEditor()
{
    return new CustomStandaloneEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CustomStandaloneProcessor();
}
