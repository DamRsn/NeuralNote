#pragma once

#include <JuceHeader.h>

#include "DownSampler.h"
#include "Parameters.h"
#include "ProcessorBase.h"

class Audio2MidiAudioProcessor : public PluginHelpers::ProcessorBase
{
public:
    Audio2MidiAudioProcessor();

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

private:
    Parameters parameters;

    DownSampler mDownSampler;

    std::vector<float> mAudioToConvert;

    const double mBasicPitchSampleRate = 22050.0;
    const double mMaxDuration = 3 * 60;
    const size_t mMaxNumSamplesToConvert =
        static_cast<size_t>(mBasicPitchSampleRate * mMaxDuration);
};
