#include "PluginProcessor.h"
#include "PluginEditor.h"

NewPluginTemplateAudioProcessor::NewPluginTemplateAudioProcessor()
{
    parameters.add(*this);
}

void NewPluginTemplateAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                                   juce::MidiBuffer& midiMessages)

{
    juce::ignoreUnused(midiMessages);

    if (parameters.enable->get())
        buffer.applyGain(parameters.gain->get());
    else
        buffer.clear();
}

juce::AudioProcessorEditor* NewPluginTemplateAudioProcessor::createEditor()
{
    return new NewPluginTemplateAudioProcessorEditor(*this);
}

void NewPluginTemplateAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
}

void NewPluginTemplateAudioProcessor::setStateInformation(const void* data,
                                                          int sizeInBytes)
{

}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NewPluginTemplateAudioProcessor();
}
