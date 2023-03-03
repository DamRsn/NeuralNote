#pragma once

#include <shared_plugin_helpers/shared_plugin_helpers.h>

//Inhereting from PluginHelpers::ProcessorBase, which is just inhereting from juce::AudioProcessor
//And adding some default implementations

class DummySynth : public PluginHelpers::ProcessorBase
{
public:
    DummySynth();

private:
    void processBlock(juce::AudioBuffer<float>& buffer,
                      juce::MidiBuffer& midiMessages) override;

    static BusesProperties getBuses();
};
