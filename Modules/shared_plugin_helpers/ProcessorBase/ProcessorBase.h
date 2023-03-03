#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

namespace PluginHelpers
{
//A helper base class, reducing a lot of the AudioProcessor boiler plate:

struct ProcessorBase : juce::AudioProcessor
{
    ProcessorBase();
    explicit ProcessorBase(const BusesProperties& ioLayouts);

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    using AudioProcessor::processBlock;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

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
}

