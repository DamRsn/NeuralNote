#pragma once

#include "atomic"
#include <JuceHeader.h>

#include "DownSampler.h"
#include "ProcessorBase.h"

struct UIParameters
{
    std::atomic<float> noteSegmentationThreshold;
    std::atomic<float> modelConfidenceThreshold;
    std::atomic<float> minNoteDurationMs;
    std::atomic<bool> recordOn;
};

enum State
{
    EmptyAudioAndMidiRegion = 0,
    Recording,
    PopulatedAudioAndMidiRegion
};

class Audio2MidiAudioProcessor : public PluginHelpers::ProcessorBase
{
public:
    Audio2MidiAudioProcessor();

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    State getState() const { return mState; };

    UIParameters mParameters;

private:
    DownSampler mDownSampler;

    State mState = EmptyAudioAndMidiRegion;

    std::vector<float> mAudioToConvert;

    const double mBasicPitchSampleRate = 22050.0;
    const double mMaxDuration = 3 * 60;
    const size_t mMaxNumSamplesToConvert =
        static_cast<size_t>(mBasicPitchSampleRate * mMaxDuration);
};
