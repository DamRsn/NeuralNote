//
// Created by Damien Ronssin on 02.06.2024.
//

#ifndef TranscriptionManager_h
#define TranscriptionManager_h

#include <JuceHeader.h>
#include "BasicPitch.h"
#include "NoteOptions.h"
#include "RhythmOptions.h"
#include "ParameterHelpers.h"

class NeuralNoteAudioProcessor;
class NeuralNoteMainView;
class NeuralNoteEditor;

class TranscriptionManager
    : public juce::Timer
    , public AudioProcessorValueTreeState::Listener
{
public:
    explicit TranscriptionManager(NeuralNoteAudioProcessor* inProcessor);

    void timerCallback() override;

    void setLauchNewTranscription();

    void launchTranscribeJob();

    void parameterChanged(const juce::String& parameterID, float newValue) override;

    bool isJobRunningOrQueued() const;

    const std::vector<Notes::Event>& getNoteEventVector() const;

    RhythmOptions& getRhythmOptions();

    void clear();

    void setMidiFileTempo(double inMidiFileTempo);

    double getMidiFileTempo() const;

private:
    void _runModel();

    void _updateTranscription();

    void _updatePostProcessing();

    void _repaintPianoRoll();

    NeuralNoteAudioProcessor* mProcessor;

    BasicPitch mBasicPitch;
    NoteOptions mNoteOptions;
    RhythmOptions mRhythmOptions;

    std::vector<Notes::Event> mPostProcessedNotes;

    std::atomic<bool> mShouldRunNewTranscription = false;
    std::atomic<bool> mShouldUpdateTranscription = false;
    std::atomic<bool> mShouldUpdatePostProcessing = false;
    std::atomic<bool> mShouldRepaintPianoRoll = false;

    juce::ThreadPool mThreadPool;
    std::function<void()> mJobLambda;

    double mMidiFileTempo = 120.0;
};

#endif //TranscriptionManager_h
