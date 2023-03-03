#pragma once

#include "PluginProcessor.h"

class PluginWithCustomModuleEditor : public juce::AudioProcessorEditor
{
public:
    explicit PluginWithCustomModuleEditor(PluginWithCustomModule&);

    void paint(juce::Graphics&) override;

};
