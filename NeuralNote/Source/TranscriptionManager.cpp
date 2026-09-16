//
// Created by Damien Ronssin on 02.06.2024.
//

#include "TranscriptionManager.h"
#include "PluginProcessor.h"
#include "NeuralNoteMainView.h"
#include "InstrumentSelection.h"
#include "NNFileUtils.h"
#include "NnGlobalSettings.h"
#include "TranscriptionConstants.h"

TranscriptionManager::TranscriptionManager(NeuralNoteAudioProcessor* inProcessor)
    : mProcessor(inProcessor)
    , mThreadPool(1)
{
    mJobLambda = [this] { _runModel(); };

    startTimerHz(30);
}

TranscriptionManager::~TranscriptionManager()
{
    stopTimer();

    // ~ThreadPool waits 5 s and then kills the thread by force, which a transcription easily
    // outlasts. Cancel and wait here instead, long enough for a cold model load or a chunk.
    mMuscriptorEngine.cancel();

    const bool jobs_finished = mThreadPool.removeAllJobs(true, 30000);
    jassertquiet(jobs_finished);
}

void TranscriptionManager::timerCallback()
{
    _catchUpSynthInstrumentsOnceFontReady();

    const auto finished_job_outcome = mFinishedJobOutcome.exchange(JobOutcome::None);

    if (finished_job_outcome != JobOutcome::None) {
        _handleFinishedJob(finished_job_outcome);
        return;
    }

    if (mJobActive && mMuscriptorEngine.drainNewNotes(mRawNotes, mFinalizedThrough)) {
        // A chunk landed. Gated on the drain having something, so this runs at the model's pace --
        // once per 5 s of audio -- rather than at the timer's.
        _updatePostProcessing();
        _repaintPianoRoll();
    }
}

void TranscriptionManager::_runModel()
{
    const auto outcome = mMuscriptorEngine.transcribeToMIDI(
        mJobModelSize,
        mProcessor->getSourceAudioManager()->getDownsampledSourceAudioForTranscription().getWritePointer(0),
        mProcessor->getSourceAudioManager()->getNumSamplesDownAcquired(),
        mJobInstruments);

    if (outcome != MuscriptorEngine::Outcome::Success) {
        mFinishedJobOutcome =
            outcome == MuscriptorEngine::Outcome::Cancelled ? JobOutcome::Cancelled : JobOutcome::Failed;
        return;
    }

    // Post-processing and the synth handoff belong to the message thread, which has been doing
    // both per chunk while this ran. All that is left here is to say so.
    //
    // Published last: the message thread treats this as the signal that the job is done with
    // everything it owns, and clearing while a job is in flight would race with it.
    mFinishedJobOutcome = JobOutcome::Success;
}

void TranscriptionManager::_handleFinishedJob(JobOutcome inOutcome)
{
    jassert(MessageManager::getInstance()->isThisTheMessageThread());

    // The job published its outcome as its last act, so nothing on the pool thread can touch the
    // state this manager owns from here on. Dropped before clear() runs below, which asserts it.
    mJobActive = false;

    switch (inOutcome) {
        case JobOutcome::Success:
            // Replaces the accumulation rather than extending it: transcribe()'s own result is
            // authoritative, and the streamed one is missing any note the model never closed.
            mRawNotes = mMuscriptorEngine.takeFinalNotes();
            mFinalizedThrough = mProcessor->getSourceAudioManager()->getAudioSampleDuration();
            _updatePostProcessing();
            mProcessor->setStateToPopulatedAudioAndMidiRegions();
            _repaintPianoRoll();
            break;

        case JobOutcome::Cancelled:
            // Back to where the transcribe button was, with the audio still loaded: cancelling a
            // run the user misconfigured should not cost them the file as well.
            mProcessor->clearTranscription();
            break;

        case JobOutcome::Failed: {
            // Read before clearTranscription(), which resets the engine and with it the message.
            // Shown rather than only pointing at the log: it is one line, and the log lives in
            // /tmp, which is neither discoverable nor durable.
            const auto reason = mMuscriptorEngine.getLastErrorMessage();

            mProcessor->clearTranscription();

            const String message = reason.empty()
                                       ? String("The transcription model could not be loaded or run.")
                                       : "The transcription model could not be loaded or run: " + String(reason) + ".";

            NativeMessageBox::showMessageBoxAsync(MessageBoxIconType::NoIcon, "Transcription failed.", message);
            break;
        }

        case JobOutcome::None:
            jassertfalse;
            break;
    }
}

void TranscriptionManager::_updatePostProcessing()
{
    jassert(mProcessor->hasTranscription());

    if (mProcessor->hasTranscription()) {
        mPostProcessedNotes = mRawNotes;

        NoteEvent::mergeOverlappingNotesWithSamePitch(mPostProcessedNotes);

        // Before the notes reach the scheduler, so no note can arrive at the synth for an
        // instrument that has no player yet. Creating one allocates and touches the list the audio
        // thread walks, so it happens here on the message thread rather than on demand.
        _ensureSynthInstruments();

        // After the synth has players for them, so the faders it pushes land somewhere.
        mProcessor->getInstrumentMixer()->rebuildFromNotes(mPostProcessedNotes);

        // For the synth. A copy, because mPostProcessedNotes is what the piano roll draws and the
        // scheduler takes ownership of what it is given.
        auto notes_to_play = mPostProcessedNotes;
        mProcessor->getPlayer()->getSynthController()->setNotes(notes_to_play);
    }
}

void TranscriptionManager::_ensureSynthInstruments()
{
    jassert(MessageManager::getInstance()->isThisTheMessageThread());

    auto* synth = mProcessor->getPlayer()->getInstrumentSynth();

    // A transcription streams in, so the set of instruments grows as it decodes; this runs once per
    // chunk and adds whatever is new. Scanning the whole note list each time is a pass over
    // something the piano roll redraws entirely anyway, and it keeps the "which instruments exist"
    // question answerable from the notes alone rather than from a second, driftable record.
    std::array<bool, NUM_INSTRUMENT_IDS> seen {};

    for (const NoteEvent& note: mPostProcessedNotes) {
        if (note.program >= 0 && note.program < NUM_INSTRUMENT_IDS) {
            seen[static_cast<std::size_t>(note.program)] = true;
        }
    }

    // Under the callback lock, for the same reason setNotes takes it: this appends to the list the
    // audio thread walks every block.
    const ScopedLock sl(mProcessor->getCallbackLock());

    for (std::size_t program = 0; program < seen.size(); program++) {
        if (seen[program]) {
            synth->ensureInstrument(static_cast<int>(program));
        }
    }
}

void TranscriptionManager::_catchUpSynthInstrumentsOnceFontReady()
{
    if (mDidCatchUpSynthInstruments || !mProcessor->hasTranscription()) {
        return;
    }

    if (mProcessor->getPlayer()->getInstrumentSynth()->isReady()) {
        // Whatever _ensureSynthInstruments has already run so far may have found the synth not
        // ready yet and done nothing; this repeats it exactly once now that it is, for every
        // instrument the current notes name. A no-op for any instrument that already exists.
        _ensureSynthInstruments();
        mDidCatchUpSynthInstruments = true;
    }
}

const std::vector<NoteEvent>& TranscriptionManager::getNoteEventVector() const
{
    return mPostProcessedNotes;
}

float TranscriptionManager::getTranscriptionProgress() const
{
    return mMuscriptorEngine.getProgress();
}

double TranscriptionManager::getFinalizedThrough() const
{
    return mFinalizedThrough;
}

void TranscriptionManager::cancelTranscription()
{
    mMuscriptorEngine.cancel();
}

void TranscriptionManager::clear()
{
    // Resets state a running job owns, so it must not be called while one is in flight. Every
    // path here either runs before the job is launched or after _handleFinishedJob.
    jassert(!mJobActive.load());

    mMuscriptorEngine.reset();
    mFinishedJobOutcome = JobOutcome::None;
    mRawNotes.clear();
    mRawNotes.shrink_to_fit();
    mFinalizedThrough = 0.0;
    mPostProcessedNotes.clear();
    mProcessor->getInstrumentMixer()->clear();

    // Otherwise the synth keeps playing the notes of the transcription that was just thrown away.
    std::vector<NoteEvent> no_notes;
    mProcessor->getPlayer()->getSynthController()->setNotes(no_notes);
    mProcessor->getPlayer()->getSynthController()->stopAllNotes();

    {
        // Drops the instruments and their faders with the transcription that named them. Under the
        // callback lock: it destroys what the audio thread renders from.
        const ScopedLock sl(mProcessor->getCallbackLock());
        mProcessor->getPlayer()->getInstrumentSynth()->reset();
    }
}

void TranscriptionManager::launchTranscribeJob()
{
    jassert(MessageManager::getInstance()->isThisTheMessageThread());

    // The Transcribe button only hides on the next timer tick, so a second click can arrive while
    // the first job is already running. Everything below writes state that job owns.
    if (mProcessor->getState() != AudioLoaded || mJobActive.load()) {
        return;
    }

    // The stored size is a preference: when that checkpoint is missing and another is there, the
    // run uses the one that is there.
    const std::optional<ModelSize> model_size = NNFileUtils::getInstalledModelSize(NnGlobalSettings::getModelSize());

    // The checkpoint went since the button was shown. The model panel's next poll replaces the button.
    if (!model_size.has_value()) {
        return;
    }

    // Armed before the Processing state is published, not after: that state is what makes the
    // cancel button live, and reset() is what clears a pending cancellation request. Doing it in
    // this order means no click can be discarded, without having to argue about which thread
    // observes what.
    mMuscriptorEngine.reset();

    // Nothing is transcribed yet, and the piano roll is about to start drawing what arrives.
    mRawNotes.clear();
    mFinalizedThrough = 0.0;
    mPostProcessedNotes.clear();

    // Read here, not in the job: the selection lives on the state tree, which belongs to this
    // thread. The job only ever sees the copy.
    mJobInstruments = InstrumentSelection::get(mProcessor->getValueTree());

    // Every instrument this run finds is a new one, and should start at unity and unmuted rather
    // than inherit a fader from whatever was transcribed before.
    mProcessor->getInstrumentMixer()->resetStoredSettings();

    mJobModelSize = *model_size;

    mProcessor->setStateToProcessing();

    // Have at least one second to transcribe
    if (mProcessor->getSourceAudioManager()->getNumSamplesDownAcquired() >= 1 * TRANSCRIPTION_SAMPLE_RATE) {
        mJobActive = true;
        mThreadPool.addJob(mJobLambda);
    } else {
        mProcessor->clear();
    }
}

void TranscriptionManager::_repaintPianoRoll()
{
    auto* main_view = mProcessor->getNeuralNoteMainView();

    if (main_view) {
        main_view->repaintPianoRoll();
    }
}
