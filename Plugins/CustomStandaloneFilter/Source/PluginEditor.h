#pragma once

#include <shared_plugin_helpers/shared_plugin_helpers.h>

class CustomStandaloneProcessor;

class CustomStandaloneEditor : public juce::AudioProcessorEditor
{
public:
    explicit CustomStandaloneEditor(CustomStandaloneProcessor&);

    void paint(juce::Graphics&) override;
    void resized() override;
};
