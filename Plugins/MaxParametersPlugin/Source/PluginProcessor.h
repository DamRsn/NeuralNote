#pragma once

#include <shared_plugin_helpers/shared_plugin_helpers.h>

//Inhereting from PluginHelpers::ProcessorBase, which is just inhereting from juce::AudioProcessor
//And adding some default implementations

class MaxParamsProcessor : public PluginHelpers::ProcessorBase
{
public:
    MaxParamsProcessor();

    void processBlock(juce::AudioBuffer<float>& buffer,
                      juce::MidiBuffer& midiMessages) override;

    bool hasEditor() const override;
    juce::AudioProcessorEditor* createEditor() override;
};
