//
// Created by Damien Ronssin on 02.06.2024.
//

#ifndef TranscriptionManager_h
#define TranscriptionManager_h

#include <JuceHeader.h>
#include "BasicPitch.h"
#include "NoteOptions.h"
#include "TimeQuantizeOptions.h"

class NeuralNoteAudioProcessor;
class NeuralNoteMainView;
class NeuralNoteEditor;

class TranscriptionManager
    : public Timer
    , public AudioProcessorValueTreeState::Listener
{
public:
    explicit TranscriptionManager(NeuralNoteAudioProcessor* inProcessor);

    void timerCallback() override;

    void processBlock();

    void setLauchNewTranscription();

    void launchTranscribeJob();

    void parameterChanged(const juce::String& parameterID, float newValue) override;

    bool isJobRunningOrQueued() const;

    const std::vector<Notes::Event>& getNoteEventVector() const;

    TimeQuantizeOptions& getTimeQuantizeOptions();

    void clear();

    void setMidiFileTempo(double inMidiFileTempo);

    double getMidiFileTempo() const;

    void saveStateToValueTree();

private:
    void _runModel();

    void _updateTranscription();

    void _updatePostProcessing();

    void _repaintPianoRoll();

    NeuralNoteAudioProcessor* mProcessor;

    BasicPitch mBasicPitch;
    NoteOptions mNoteOptions;
    TimeQuantizeOptions mTimeQuantizeOptions;

    std::vector<Notes::Event> mPostProcessedNotes;

    std::atomic<bool> mShouldRunNewTranscription = false;
    std::atomic<bool> mShouldUpdateTranscription = false;
    std::atomic<bool> mShouldUpdatePostProcessing = false;
    std::atomic<bool> mShouldRepaintPianoRoll = false;

    ThreadPool mThreadPool;
    std::function<void()> mJobLambda;

    double mMidiFileTempo = 120.0;
};

#endif //TranscriptionManager_h
