#pragma once

#include "atomic"
#include <JuceHeader.h>

#include "DownSampler.h"
#include "ProcessorBase.h"
#include "BasicPitch.h"
#include "NoteOptions.h"

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
    struct Parameters
    {
        std::atomic<float> noteSensibility = 0.5;
        std::atomic<float> splitSensibility = 0.7;
        std::atomic<float> minNoteDurationMs = 125;
        std::atomic<int> pitchBendMode = 0;

        std::atomic<bool> useNoteOptions = true;
        std::atomic<int> minMidiNote = MIN_MIDI_NOTE;
        std::atomic<int> maxMidiNote = MAX_MIDI_NOTE;
        std::atomic<int> keyRootNote = 0;
        std::atomic<int> keyType = 0;
        std::atomic<int> keySnapMode = 0;

        std::atomic<bool> useRhythmOptions = true;
        std::atomic<int> rhythmTimeDivision = 0;
        std::atomic<float> rhythmQuantizationForce = 0;
    };

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

    void updateTranscription();

    void updatePostProcessing();

    Parameters* getCustomParameters();

private:
    void _runModel();

    std::atomic<State> mState = EmptyAudioAndMidiRegions;

    AudioBuffer<float> mMonoBuffer;
    DownSampler mDownSampler;

    Parameters mParameters;
    bool mWasRecording = false;

    AudioBuffer<float> mAudioBufferForMIDITranscription;

    BasicPitch mBasicPitch;
    NoteOptions mNoteOptions;

    std::vector<Notes::Event> mPostProcessedNotes;

    // Threading for running ML in background thread.
    juce::ThreadPool mThreadPool;
    std::function<void()> mJobLambda;

    int mNumSamplesAcquired = 0;
    const double mBasicPitchSampleRate = 22050.0;
    const double mMaxDuration = 3 * 60;
    const int mMaxNumSamplesToConvert =
        static_cast<int>(mBasicPitchSampleRate * mMaxDuration);
};
