#include "PluginProcessor.h"

void MinimalAudioPlugin::processBlock(juce::AudioBuffer<float>& buffer,
                                      juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    buffer.clear();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MinimalAudioPlugin();
}
