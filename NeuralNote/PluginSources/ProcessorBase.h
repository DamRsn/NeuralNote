#pragma once

#include <JuceHeader.h>

namespace PluginHelpers
{
//A helper base class, reducing a lot of the AudioProcessor boiler plate:

struct ProcessorBase : juce::AudioProcessor {
    ProcessorBase();
    explicit ProcessorBase(const BusesProperties& ioLayouts);

    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    using AudioProcessor::prepareToPlay;

    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    static BusesProperties getDefaultProperties();
};
} // namespace PluginHelpers
