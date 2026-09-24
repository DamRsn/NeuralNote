//
// Created by Damien Ronssin on 02.06.2024.
//

#ifndef TranscriptionManager_h
#define TranscriptionManager_h

#include <optional>

#include <JuceHeader.h>
#include "MuscriptorEngine.h"
#include "TranscriptionConstants.h"

class NeuralNoteAudioProcessor;
class NeuralNoteMainView;
class NeuralNoteEditor;

class TranscriptionManager : public Timer
{
public:
    explicit TranscriptionManager(NeuralNoteAudioProcessor* inProcessor);

    ~TranscriptionManager() override;

    void timerCallback() override;

    void launchTranscribeJob();

    const std::vector<NoteEvent>& getNoteEventVector() const;

    /** @return Phase and progress of the current/last transcription. */
    MuscriptorEngine::Progress getTranscriptionProgress() const;

    /**
     * @return Time in seconds below which the note vector is complete, not merely correct. While a
     *         transcription runs it is the decode frontier: everything before it can be played and
     *         drawn, and nothing is known after it. Equal to the audio duration once finished.
     */
    double getFinalizedThrough() const;

    /**
     * Requests that an in-flight transcription stop as soon as possible.
     */
    void cancelTranscription();

    void clear();

    /** @return The checkpoint the current transcription is from, running or finished. Message thread. */
    std::optional<ModelSize> getTranscriptionModelSize() const;

    /**
     * @return The finished transcription as an NnId::TranscriptionId tree, or an invalid tree when
     *         there is none. Safe from any thread: reads only the pre-serialised notes, under a lock.
     */
    ValueTree createStateTree() const;

    /**
     * Makes inTree the current transcription, landing in PopulatedAudioAndMidiRegions without going
     * through Processing. An invalid or unreadable tree leaves no transcription. Expects the audio it
     * was made from to be loaded already, and does nothing while a job is running. Message thread.
     */
    void restoreFromStateTree(const ValueTree& inTree);

private:
    /**
     * Outcome of the last transcription job, as published by the thread pool thread and consumed
     * by the message thread. None means there is nothing left to apply.
     */
    enum class JobOutcome { None, Success, Cancelled, Failed };

    void _runModel();

    /**
     * Applies a finished job's outcome. Message thread only: everything downstream of it (the
     * processor state, the value tree, the UI) belongs to that thread.
     */
    void _handleFinishedJob(JobOutcome inOutcome);

    /**
     * Re-derives the played and drawn notes from the raw ones and fans them out to the synth, the
     * mixer and the piano roll. Runs per decoded chunk, so a partial transcription is playable.
     */
    void _updatePostProcessing();

    /** Gives the synth a player for every instrument in the current notes. Message thread. */
    void _ensureSynthInstruments();

    /**
     * Catches up once the soundfont finishes loading, in case it was still loading the last time
     * _ensureSynthInstruments ran. It normally is not: a transcription takes minutes and the font
     * loads in about a second. Without this, a transcription that finishes inside that window would
     * leave every instrument silent for good, because nothing else re-scans the note list once the
     * font becomes ready.
     */
    void _catchUpSynthInstrumentsOnceFontReady();

    void _repaintPianoRoll();

    /** Serialises mRawNotes for createStateTree. Message thread, once per finished transcription. */
    void _updateSavedNotes();

    NeuralNoteAudioProcessor* mProcessor;

    MuscriptorEngine mMuscriptorEngine;

    // What the running job was launched with. Both are written by launchTranscribeJob before the
    // job is queued and read only by the job, so they need no synchronisation of their own.
    std::vector<msl::InstrumentGroup> mJobInstruments;
    ModelSize mJobModelSize = DEFAULT_MODEL_SIZE;

    // The model's own output, accumulated chunk by chunk as the job decodes it. Message thread
    // only, and owned here rather than by the engine because it has to outlive it: the engine is
    // reset between runs, and this is what post-processing re-derives from and what the plugin
    // state persists.
    std::vector<NoteEvent> mRawNotes;

    // Message thread only. Set from the moment a job is launched, so it names a partial transcription too.
    std::optional<ModelSize> mTranscriptionModelSize;

    // mRawNotes as NnId::TranscriptionNotesId stores them, with the model that produced them. Only
    // set once a transcription has finished. Serialised ahead of time rather than in
    // createStateTree because hosts can ask for the state from any thread, and often.
    struct SavedNotes {
        String notesJson;
        ModelSize modelSize;
    };

    std::optional<SavedNotes> mSavedNotes;
    mutable CriticalSection mSavedNotesLock;

    // How far mRawNotes is complete; see getFinalizedThrough.
    double mFinalizedThrough = 0.0;

    std::vector<NoteEvent> mPostProcessedNotes;

    std::atomic<JobOutcome> mFinishedJobOutcome = JobOutcome::None;

    // True from the moment a job is queued until the message thread has applied its outcome. Only
    // there to make clear()'s "no job may be in flight" precondition assertable: it cannot be
    // derived from mThreadPool, which still reports the job as running when the outcome lands.
    std::atomic<bool> mJobActive = false;

    // Set once _catchUpSynthInstrumentsOnceFontReady has done its one catch-up pass, so it does not
    // rescan the note list on every timer tick for the rest of the session.
    bool mDidCatchUpSynthInstruments = false;

    ThreadPool mThreadPool;
    std::function<void()> mJobLambda;
};

#endif //TranscriptionManager_h
