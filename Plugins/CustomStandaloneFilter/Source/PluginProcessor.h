#pragma once

#include <shared_plugin_helpers/shared_plugin_helpers.h>

class CustomStandaloneProcessor : public PluginHelpers::ProcessorBase
{
private:
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;

};
