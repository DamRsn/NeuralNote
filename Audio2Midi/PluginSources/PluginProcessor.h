#pragma once

#include "atomic"
#include <JuceHeader.h>

#include "DownSampler.h"
#include "ProcessorBase.h"

struct UIParameters
{
    std::atomic<float> noteSegmentationThreshold = 0.5;
    std::atomic<float> modelConfidenceThreshold = 0.5;
    std::atomic<float> minNoteDurationMs = 11;
    std::atomic<bool> recordOn = false;
};

enum State
{
    EmptyAudioAndMidiRegions = 0,
    Recording,
    Processing,
    PopulatedAudioAndMidiRegions
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

    State getState() const { return mState.load(); };

    UIParameters mParameters;

private:
    DownSampler mDownSampler;

    std::atomic<State> mState = EmptyAudioAndMidiRegions;

    std::vector<float> mAudioToConvert;

    juce::ThreadPool mThreadPool;

    int mIndex = 0;
    const double mBasicPitchSampleRate = 22050.0;
    const double mMaxDuration = 10;
    const size_t mMaxNumSamplesToConvert =
        static_cast<size_t>(mBasicPitchSampleRate * mMaxDuration);
};
