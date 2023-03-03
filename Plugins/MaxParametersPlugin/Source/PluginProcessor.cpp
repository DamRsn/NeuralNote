#include "PluginProcessor.h"

constexpr int numParams = 10000;
constexpr bool useEditor = false;

MaxParamsProcessor::MaxParamsProcessor()
{
    for (int index = 0; index < numParams; ++index)
    {
        auto name = juce::String(index);
        addParameter(new juce::AudioParameterBool({name, 1}, name, false));
    }
}

void MaxParamsProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                      juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    buffer.clear();
}
juce::AudioProcessorEditor* MaxParamsProcessor::createEditor()
{
    if (useEditor)
        return PluginHelpers::ProcessorBase::createEditor();
    else
       return nullptr;
}

bool MaxParamsProcessor::hasEditor() const
{
    return useEditor;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MaxParamsProcessor();
}
