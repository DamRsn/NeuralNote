#include "PluginProcessor.h"

DummySynth::DummySynth()
    : PluginHelpers::ProcessorBase(getBuses())
{
}

void DummySynth::processBlock(juce::AudioBuffer<float>& buffer,
                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    buffer.clear();
}

juce::AudioProcessor::BusesProperties DummySynth::getBuses()
{
    juce::AudioProcessor::BusesProperties properties;
    auto stereo = juce::AudioChannelSet::stereo();
    properties = properties.withOutput("Output", stereo);
    properties = properties.withOutput("Extra", stereo);

    return properties;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DummySynth();
}
