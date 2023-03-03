#pragma once

#include "Parameters.h"

class NewPluginTemplateAudioProcessor : public PluginHelpers::ProcessorBase
{
public:
    NewPluginTemplateAudioProcessor();

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

private:

    Parameters parameters;
};
