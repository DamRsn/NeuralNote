#pragma once

#include "atomic"
#include <JuceHeader.h>

#include "DownSampler.h"
#include "ProcessorBase.h"
#include "BasicPitch.h"

struct UIParameters
{
    std::atomic<float> noteSegmentationThreshold = 0.5;
    std::atomic<float> modelConfidenceThreshold = 0.5;
    std::atomic<float> minNoteDurationMs = 11;
    //    std::atomic<bool> recordOn = false;
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

    State getState() const { return mState.load(); }

    void setStateToRecording() { mState.store(Recording); }

    void setStateToProcessing() { mState.store(Processing); }

    void clear();

    AudioBuffer<float>& getAudioBufferForMidi();

    int getNumSamplesAcquired() const;

    void setNumSamplesAcquired(int inNumSamplesAcquired);

    void launchTranscribeJob();

    const std::vector<Notes::Event>& getNoteEventVector() const;

    UIParameters mParameters;

private:
    void _runModel();

    DownSampler mDownSampler;

    bool mWasRecording = false;

    std::atomic<State> mState = EmptyAudioAndMidiRegions;

    AudioBuffer<float> mAudioBufferForMIDITranscription;

    BasicPitch mBasicPitch;

    juce::ThreadPool mThreadPool;
    std::function<void()> mJobLambda;

    int mNumSamplesAcquired = 0;
    const double mBasicPitchSampleRate = 22050.0;
    const double mMaxDuration = 3 * 60;
    const int mMaxNumSamplesToConvert =
        static_cast<int>(mBasicPitchSampleRate * mMaxDuration);
};
