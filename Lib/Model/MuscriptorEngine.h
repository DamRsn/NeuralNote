//
// Created by Damien Ronssin on 03.08.26.
//

#ifndef MuscriptorEngine_h
#define MuscriptorEngine_h

#include <atomic>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "muscriptor/muscriptor.hpp"

#include "NoteEvent.h"
#include "TranscriptionConstants.h"

/**
 * Wraps msl::Transcriber. Runs a blocking transcription and converts the result to NoteEvent for
 * the rest of the plugin.
 *
 * Notes are published as the model decodes them, not only when it finishes, so the plugin can fill
 * the piano roll and play what is already transcribed while the rest is still running. The engine
 * owns only the staging area between the job thread and whoever drains it; the accumulated
 * transcription belongs to the caller, which outlives the engine.
 *
 * The model does not outlive the transcription that loaded it: weights plus KV cache are over a
 * gigabyte resident at medium/F16, which is not something a plugin should hold while sitting idle
 * in a DAW. Reloading takes about 0.3 s at medium, negligible next to a transcription that runs
 * slower than real time.
 */
class MuscriptorEngine
{
public:
    /**
     * How a transcribeToMIDI call ended. Cancelled is kept distinct from Failed because the two
     * want different handling: one is the user's own doing, the other is worth reporting.
     */
    enum class Outcome { Success, Cancelled, Failed };

    enum class Phase : std::uint8_t { LoadingModel, Transcribing };

    /** Where a transcribeToMIDI call is, as one value so the two fields are always read together. */
    struct Progress {
        Phase phase = Phase::LoadingModel;
        // In [0, 1] within the phase, or negative while the phase has nothing to measure yet: GPU
        // initialisation comes before the first weight is read, and can take many seconds.
        float fraction = -1.0f;
    };

    MuscriptorEngine() = default;

    /**
     * Drops everything the last transcription produced and arms the engine for a new one: pending
     * cancellation and progress are reset here, and deliberately nowhere else. Must be called
     * before every transcribeToMIDI call, from the thread that owns the job's lifetime, while no
     * job is in flight. Nothing to unload: the model is never held between transcriptions.
     */
    void reset();

    /**
     * Transcribes the input audio. Blocking, can take from seconds to minutes: must be called
     * off the message/audio thread, and only after reset(). Loads the model on entry and unloads
     * it before returning, whatever the outcome.
     *
     * Notes reach the caller two ways. drainNewNotes() yields them chunk by chunk as they are
     * decoded, from another thread, while this is still running; takeFinalNotes() yields the whole
     * transcription once this has returned Success. The second is authoritative -- see the note on
     * drainNewNotes.
     *
     * The notes are the model's own output: nothing is merged, filtered or otherwise
     * post-processed here.
     * @param inModelSize Which checkpoint to load from the models directory.
     * @param inAudio Pointer to raw audio (must be mono at TRANSCRIPTION_SAMPLE_RATE Hz).
     * @param inNumSamples Number of input samples available.
     * @param inInstruments Which instruments to look for. Empty lets the model choose, which is
     *        what every run did before the picker existed. A non-empty selection is not a filter
     *        over the result: the library takes it as a conditioning prefix and a hard mask over
     *        what the decoder may emit, so it belongs here and not downstream.
     * @return Success only if the final note event vector is usable.
     */
    Outcome transcribeToMIDI(ModelSize inModelSize,
                             const float* inAudio,
                             int inNumSamples,
                             const std::vector<msl::InstrumentGroup>& inInstruments);

    /**
     * Moves everything decoded since the last call onto the end of ioNotes, and advances
     * ioFinalizedThrough to the time below which the transcription is now complete. Both under one
     * lock, so the horizon can never describe more than the notes handed over with it -- a caller
     * that played up to a horizon running ahead of its own note list would drop notes silently.
     *
     * Thread-safe, and meant to be called while transcribeToMIDI runs on another thread.
     *
     * What arrives here is final and append-only, with one exception the library documents: a note
     * the model never closes surfaces in the last chunk with a 10 ms duration, behind the horizon
     * already reported. takeFinalNotes() is not subject to it, which is why the finished
     * transcription should replace an accumulation rather than extend it.
     *
     * @return true if either output changed, i.e. there is something new to show.
     */
    bool drainNewNotes(std::vector<NoteEvent>& ioNotes, double& ioFinalizedThrough);

    /**
     * @return The whole transcription, moved out. Valid only after transcribeToMIDI returned
     *         Success, and only once.
     */
    std::vector<NoteEvent> takeFinalNotes();

    /**
     * @return Why the last transcribeToMIDI call returned Failed, short enough to show to the
     *         user. Empty if it did not fail. Written before the outcome is published, so it is
     *         readable by whoever observes that outcome.
     */
    const std::string& getLastErrorMessage() const;

    /**
     * Requests that the current or next transcribeToMIDI call stop as soon as possible. Holds
     * until the next reset(), so a request arriving before the job actually starts is not lost.
     * Thread-safe.
     */
    void cancel();

    /** @return Progress of the current/last transcribeToMIDI call. Thread-safe. */
    Progress getProgress() const;

private:
    /** @return Success with mTranscriber set, or why not; Failed also sets mLastErrorMessage. */
    Outcome _loadModel(ModelSize inModelSize);

    // Only holds a value for the duration of a transcribeToMIDI call.
    std::optional<msl::Transcriber> mTranscriber;

    // Job thread to whoever drains. Guards both, because the horizon is only meaningful alongside
    // the notes it was reported with. Cleared but not shrunk while a job runs, so after a chunk or
    // two the handoff stops allocating.
    std::mutex mStagingMutex;
    std::vector<NoteEvent> mStaging;
    double mFinalizedThrough = 0.0;

    // The whole transcription. Written before the outcome is published, so whoever observes that
    // outcome can read it without further synchronisation.
    std::vector<NoteEvent> mFinalNotes;

    std::string mLastErrorMessage;

    std::atomic<bool> mCancelRequested {false};
    std::atomic<Progress> mProgress {Progress {}};
};

#endif // MuscriptorEngine_h
