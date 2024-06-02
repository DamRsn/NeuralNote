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
#include "ParameterHelpers.h"

enum State { EmptyAudioAndMidiRegions = 0, Recording, Processing, PopulatedAudioAndMidiRegions };

class NeuralNoteAudioProcessor : public PluginHelpers::ProcessorBase
{
public:
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

    const std::vector<Notes::Event>& getNoteEventVector() const;

    void updateTranscription();

    void updatePostProcessing();

    const juce::Optional<juce::AudioPlayHead::PositionInfo>&
        getPlayheadInfoOnRecordStart(); // TODO: Add to timeQuantizeManager

    // Value tree state to pass automatable parameters from UI
    juce::AudioProcessorValueTreeState mAPVTS;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // TODO: TimeQuantizeManager
    bool canQuantize() const;

    std::string getTempoStr() const;

    std::string getTimeSignatureStr() const;

    void setMidiFileTempo(double inMidiFileTempo);

    double getMidiFileTempo() const;

    SourceAudioManager* getSourceAudioManager();

    Player* getPlayer();

    RhythmOptions* getRhythmOptions();

    std::array<RangedAudioParameter*, ParameterHelpers::TotalNumParams>& getParams();

    float getParameterValue(ParameterHelpers::ParamIdEnum inParamId) const;

private:
    void _runModel(); // Add to TranscriptionManager

    std::array<RangedAudioParameter*, ParameterHelpers::TotalNumParams> mParams {};

    std::atomic<State> mState = EmptyAudioAndMidiRegions;

    std::unique_ptr<SourceAudioManager> mSourceAudioManager;
    std::unique_ptr<Player> mPlayer;

    bool mWasRecording = false;

    std::atomic<double> mCurrentTempo = -1.0;
    std::atomic<int> mCurrentTimeSignatureNum = -1;
    std::atomic<int> mCurrentTimeSignatureDenom = -1;

    double mMidiFileTempo = 120.0;

    // To transcription manager
    BasicPitch mBasicPitch;
    NoteOptions mNoteOptions;
    RhythmOptions mRhythmOptions;

    std::vector<Notes::Event> mPostProcessedNotes;

    // To quantize manager
    juce::Optional<juce::AudioPlayHead::PositionInfo> mPlayheadInfoStartRecord;

    // Thread pool to run ML in background thread. To transcription manager
    juce::ThreadPool mThreadPool;
    std::function<void()> mJobLambda;
};
