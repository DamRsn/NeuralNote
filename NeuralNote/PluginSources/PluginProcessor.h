#pragma once

#include "atomic"
#include <JuceHeader.h>

#include "Resampler.h"
#include "ProcessorBase.h"
#include "BasicPitch.h"
#include "NoteOptions.h"
#include "MidiFileWriter.h"
#include "RhythmOptions.h"
#include "Player.h"
#include "SourceAudioManager.h"

enum State { EmptyAudioAndMidiRegions = 0, Recording, Processing, PopulatedAudioAndMidiRegions };

class NeuralNoteAudioProcessor : public PluginHelpers::ProcessorBase
{
public:
    struct Parameters {
        std::atomic<float> noteSensibility = 0.7;
        std::atomic<float> splitSensibility = 0.5;
        std::atomic<float> minNoteDurationMs = 125;
        std::atomic<int> pitchBendMode = NoPitchBend;

        std::atomic<int> minMidiNote = MIN_MIDI_NOTE;
        std::atomic<int> maxMidiNote = MAX_MIDI_NOTE;
        std::atomic<int> keyRootNote = NoteUtils::C;
        std::atomic<int> keyType = NoteUtils::Chromatic;
        std::atomic<int> keySnapMode = 1;

        std::atomic<int> rhythmTimeDivision = RhythmUtils::_1_8;
        std::atomic<float> rhythmQuantizationForce = 0.0f;
    };

    NeuralNoteAudioProcessor();

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    State getState() const { return mState.load(); }

    void setStateToRecording() { mState.store(Recording); }

    void setStateToProcessing() { mState.store(Processing); }

    void clear();

    // TODO: function to put in a new class TranscriptionManager
    void launchTranscribeJob();

    bool isJobRunningOrQueued() const;

    //    AudioBuffer<float>& getAudioBufferForMidi();

    const std::vector<Notes::Event>& getNoteEventVector() const;

    void updateTranscription();

    void updatePostProcessing();

    Parameters* getCustomParameters();

    const juce::Optional<juce::AudioPlayHead::PositionInfo>&
        getPlayheadInfoOnRecordStart(); // TODO: Add to timeQuantizeManager

    // Value tree state to pass automatable parameters from UI
    juce::AudioProcessorValueTreeState mTree;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // TODO: TimeQuantizeManager
    bool canQuantize() const;

    std::string getTempoStr() const;

    std::string getTimeSignatureStr() const;

    void setMidiFileTempo(double inMidiFileTempo);

    double getMidiFileTempo() const;

    SourceAudioManager* getSourceAudioManager();

    Player* getPlayer();

private:
    void _runModel(); // Add to TranscriptionManager

    std::atomic<State> mState = EmptyAudioAndMidiRegions;

    std::unique_ptr<SourceAudioManager> mSourceAudioManager;
    std::unique_ptr<Player> mPlayer;

    //    AudioBuffer<float> mMonoBuffer;
    //    Resampler mDownSampler;

    Parameters mParameters;
    bool mWasRecording = false;
    bool mIsPlayheadPlaying = false;

    std::atomic<double> mCurrentTempo = -1.0;
    std::atomic<int> mCurrentTimeSignatureNum = -1;
    std::atomic<int> mCurrentTimeSignatureDenom = -1;

    AudioBuffer<float> mAudioBufferForMIDITranscription;

    double mMidiFileTempo = 120.0;

    BasicPitch mBasicPitch;
    NoteOptions mNoteOptions;
    RhythmOptions mRhythmOptions;

    std::string mDroppedFilename;

    std::vector<Notes::Event> mPostProcessedNotes;

    juce::Optional<juce::AudioPlayHead::PositionInfo> mPlayheadInfoStartRecord;

    // Thread pool to run ML in background thread.
    juce::ThreadPool mThreadPool;
    std::function<void()> mJobLambda;

    int mNumSamplesAcquired = 0;
    //    const double mBasicPitchSampleRate = 22050.0;
    //    const double mMaxDuration = 3 * 60;
    //    const int mMaxNumSamplesToConvert = static_cast<int>(mBasicPitchSampleRate * mMaxDuration);
};
