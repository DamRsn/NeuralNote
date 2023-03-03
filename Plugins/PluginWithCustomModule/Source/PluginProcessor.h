#pragma once

#include <shared_processing_code/shared_processing_code.h>
#include <shared_plugin_helpers/shared_plugin_helpers.h>

class PluginWithCustomModule : public PluginHelpers::ProcessorBase
{
public:

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

private:
    WhiteNoise::Oscillator whiteNoise;
};
