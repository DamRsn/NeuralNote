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

namespace
{
// The layout of the saved TRANSCRIPTION tree and its notes.
constexpr int TRANSCRIPTION_FORMAT_VERSION = 1;

// The layout of the saved PARTIAL_TRANSCRIPTION tree.
constexpr int PARTIAL_TRANSCRIPTION_FORMAT_VERSION = 1;

// Note times are saved to the microsecond. JUCE trims the trailing zeros, so a time on the model's
// 10 ms grid is written as e.g. "0.33".
constexpr int SAVED_TIME_DECIMAL_PLACES = 6;

/**
 * Reads the notes notesToJson wrote. Any malformed entry rejects the whole list; a note starting
 * outside [0, inDuration] is dropped. The end is not bounded: a note left open at the end of the
 * audio ends 10 ms after its onset, which can be past the last sample.
 */
std::optional<std::vector<NoteEvent>> parseNotes(const String& inJson, double inDuration)
{
    const var parsed = JSON::parse(inJson);
    const Array<var>* entries = parsed.getArray();

    if (entries == nullptr) {
        return std::nullopt;
    }

    std::vector<NoteEvent> notes;
    notes.reserve(static_cast<std::size_t>(entries->size()));

    for (const var& entry: *entries) {
        const Array<var>* fields = entry.getArray();

        if (fields == nullptr || fields->size() != 4) {
            return std::nullopt;
        }

        NoteEvent note;
        note.startTime = fields->getReference(0);
        note.endTime = fields->getReference(1);
        note.pitch = fields->getReference(2);
        note.program = fields->getReference(3);
        note.amplitude = FIXED_NOTE_AMPLITUDE;

        const bool valid_times =
            std::isfinite(note.startTime) && std::isfinite(note.endTime) && note.endTime >= note.startTime;
        const bool valid_pitch = note.pitch >= MIN_MIDI_NOTE && note.pitch <= MAX_MIDI_NOTE;
        const bool valid_program = note.program >= 0 && note.program < NUM_INSTRUMENT_IDS;

        if (!valid_times || !valid_pitch || !valid_program) {
            return std::nullopt;
        }

        if (note.startTime >= 0.0 && note.startTime <= inDuration) {
            notes.push_back(note);
        }
    }

    return notes;
}

String notesToJson(const std::vector<NoteEvent>& inNotes)
{
    Array<var> entries;
    entries.ensureStorageAllocated(static_cast<int>(inNotes.size()));

    for (const NoteEvent& note: inNotes) {
        entries.add(Array<var> {note.startTime, note.endTime, note.pitch, note.program});
    }

    const auto format =
        JSON::FormatOptions {}.withSpacing(JSON::Spacing::none).withMaxDecimalPlaces(SAVED_TIME_DECIMAL_PLACES);

    return JSON::toString(entries, format);
}
} // namespace

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

    // ~ThreadPool waits 5 s and then kills the thread by force. Cancel and wait here instead, long
    // enough for GPU backend initialisation, which cannot be interrupted and took about 20 s on
    // the first run after a new build.
    mMuscriptorEngine.cancel();

    const bool jobs_finished = mThreadPool.removeAllJobs(true, 30000);
    jassertquiet(jobs_finished);
}

void TranscriptionManager::timerCallback()
{
    _catchUpSynthInstrumentsOnceFontReady();

    if (mJobFinished.exchange(false)) {
        _handleFinishedJob();
        return;
    }

    if (mJobActive) {
        _drainEngine();
    }
}

void TranscriptionManager::_drainEngine()
{
    // Gated on the drain having something, so this runs at the model's pace -- once per 5 s of
    // audio -- rather than at the timer's.
    if (mMuscriptorEngine.drainNewNotes(mRawNotes, mFinalizedThrough, mResumePoint, mResumeProgress)) {
        _updateSavedTranscription();
        _updatePostProcessing();
        _repaintPianoRoll();
    }
}

void TranscriptionManager::_runModel()
{
    mJobOutcome = mMuscriptorEngine.transcribeToMIDI(
        mJobModelSize,
        mJobDevice,
        mProcessor->getSourceAudioManager()->getDownsampledSourceAudioForTranscription().getWritePointer(0),
        mProcessor->getSourceAudioManager()->getNumSamplesDownAcquired(),
        mJobInstruments,
        mJobResumeFrom,
        mJobStartProgress);

    // Post-processing and the synth handoff belong to the message thread, which has been doing
    // both per chunk while this ran. All that is left here is to say so.
    //
    // Published last: the message thread treats this as the signal that the job is done with
    // everything it owns, and clearing while a job is in flight would race with it.
    mJobFinished = true;
}

void TranscriptionManager::_handleFinishedJob()
{
    using Outcome = MuscriptorEngine::Outcome;

    jassert(MessageManager::getInstance()->isThisTheMessageThread());

    // The job published its outcome as its last act, so nothing on the pool thread can touch the
    // state this manager owns from here on. Dropped before clear() runs below, which asserts it.
    mJobActive = false;

    if (mJobOutcome == Outcome::Success) {
        // Replaces what this job streamed rather than extending it: transcribe()'s own result is
        // authoritative, and the streamed one is missing any note the model never closed.
        auto final_notes = mMuscriptorEngine.takeFinalNotes();
        mRawNotes.resize(mNotesBeforeJob);
        mRawNotes.insert(mRawNotes.end(), final_notes.begin(), final_notes.end());
        mFinalizedThrough = mProcessor->getSourceAudioManager()->getAudioSampleDuration();
        mResumePoint.clear();
        mResumeProgress = 0.0f;
        _updateSavedTranscription();
        _updatePostProcessing();
        mProcessor->setStateToPopulatedAudioAndMidiRegions();
        _repaintPianoRoll();
        return;
    }

    // Read before clearTranscription(), which resets the engine and with it the message.
    const String reason = mMuscriptorEngine.getLastErrorMessage();
    bool keep = !mDiscardOnStop && mJobOutcome != Outcome::CannotResume;

    if (keep) {
        // Whatever landed between the last timer tick and the stop belongs to the paused transcription.
        _drainEngine();
        keep = !mResumePoint.empty();
    }

    if (keep) {
        mProcessor->setStateToPaused();
        _repaintPianoRoll();
    } else {
        // Back to where the transcribe button was, with the audio still loaded: stopping a run
        // the user misconfigured should not cost them the file as well.
        mProcessor->clearTranscription();
    }

    // Shown rather than only pointing at the log: it is one line, and the log lives in /tmp, which
    // is neither discoverable nor durable.
    if (mJobOutcome == Outcome::Failed) {
        String message = reason.isEmpty() ? String("The transcription model could not be loaded or run.")
                                          : "The transcription model could not be loaded or run: " + reason + ".";

        if (keep) {
            message += " What was transcribed so far is kept, and can be resumed.";
        }

        NativeMessageBox::showMessageBoxAsync(MessageBoxIconType::NoIcon, "Transcription failed.", message);
    }

    if (mJobOutcome == Outcome::CannotResume) {
        NativeMessageBox::showMessageBoxAsync(MessageBoxIconType::NoIcon,
                                              "Transcription could not be resumed.",
                                              "The paused transcription does not match the loaded audio. "
                                              "Transcribe again to start over.");
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

MuscriptorEngine::Progress TranscriptionManager::getTranscriptionProgress() const
{
    if (mProcessor->getState() == Paused) {
        return {MuscriptorEngine::Phase::Transcribing, mResumeProgress};
    }

    return mMuscriptorEngine.getProgress();
}

double TranscriptionManager::getFinalizedThrough() const
{
    return mFinalizedThrough;
}

void TranscriptionManager::pauseTranscription()
{
    mMuscriptorEngine.cancel();
}

void TranscriptionManager::discardTranscription()
{
    mDiscardOnStop = true;
    mMuscriptorEngine.cancel();
}

bool TranscriptionManager::isCancelRequested() const
{
    return mMuscriptorEngine.isCancelRequested();
}

void TranscriptionManager::clear()
{
    // Resets state a running job owns, so it must not be called while one is in flight. Every
    // path here either runs before the job is launched or after _handleFinishedJob.
    jassert(!mJobActive.load());

    mMuscriptorEngine.reset();
    mJobFinished = false;
    mRawNotes.clear();
    mRawNotes.shrink_to_fit();
    mNotesBeforeJob = 0;
    mResumePoint.clear();
    mResumeProgress = 0.0f;
    mDiscardOnStop = false;
    mFinalizedThrough = 0.0;
    mPostProcessedNotes.clear();
    mTranscriptionModelSize.reset();

    {
        const ScopedLock sl(mSavedTranscriptionLock);
        mSavedTranscription.reset();
        mSavedNotesJson.reset();
    }

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

void TranscriptionManager::startTranscription()
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

    // Have at least one second to transcribe
    if (mProcessor->getSourceAudioManager()->getNumSamplesDownAcquired() < 1 * TRANSCRIPTION_SAMPLE_RATE) {
        mProcessor->clear();
        return;
    }

    // Nothing is transcribed yet, and the piano roll is about to start drawing what arrives.
    mRawNotes.clear();
    mNotesBeforeJob = 0;
    mResumePoint.clear();
    mResumeProgress = 0.0f;
    mFinalizedThrough = 0.0;
    mPostProcessedNotes.clear();

    // Every instrument this run finds is a new one, and should start at unity and unmuted rather
    // than inherit a fader from whatever was transcribed before.
    mProcessor->getInstrumentMixer()->resetStoredSettings();

    mJobModelSize = *model_size;
    mTranscriptionModelSize = *model_size;
    mJobResumeFrom.clear();
    mJobStartProgress = 0.0f;

    _launchJob();
}

void TranscriptionManager::resumeTranscription()
{
    jassert(MessageManager::getInstance()->isThisTheMessageThread());

    if (mProcessor->getState() != Paused || mJobActive.load() || mResumePoint.empty()
        || !mTranscriptionModelSize.has_value()) {
        return;
    }

    // Another checkpoint would continue another model's transcription.
    if (!NNFileUtils::isModelInstalled(*mTranscriptionModelSize)) {
        NativeMessageBox::showMessageBoxAsync(MessageBoxIconType::NoIcon,
                                              "Model not installed.",
                                              "This transcription was started with the "
                                                  + String(modelSizeToDisplayName(*mTranscriptionModelSize))
                                                  + " model. Download it from the model menu to resume.");
        return;
    }

    // The engine's final notes replace everything this job streams, and nothing before.
    mNotesBeforeJob = mRawNotes.size();

    mJobModelSize = *mTranscriptionModelSize;
    mJobResumeFrom = mResumePoint;
    mJobStartProgress = mResumeProgress;

    _launchJob();
}

void TranscriptionManager::_launchJob()
{
    // Armed before the Processing state is published, not after: that state is what makes the
    // pause button live, and reset() is what clears a pending cancellation request. Doing it in
    // this order means no click can be discarded, without having to argue about which thread
    // observes what.
    mMuscriptorEngine.reset();
    mDiscardOnStop = false;

    // Read here, not in the job: the selection lives on the state tree, which belongs to this
    // thread. The job only ever sees the copy. It cannot change while a transcription exists, so a
    // resumed run gets the selection its resume point was made with.
    mJobInstruments = InstrumentSelection::get(mProcessor->getValueTree());
    mJobDevice = NnGlobalSettings::getComputeDevice();

    mProcessor->setStateToProcessing();

    mJobActive = true;
    mThreadPool.addJob(mJobLambda);
}

std::optional<ModelSize> TranscriptionManager::getTranscriptionModelSize() const
{
    return mTranscriptionModelSize;
}

ValueTree TranscriptionManager::createStateTree() const
{
    const ScopedLock sl(mSavedTranscriptionLock);

    if (!mSavedTranscription.has_value()) {
        return {};
    }

    const SavedTranscription& saved = *mSavedTranscription;

    if (!mSavedNotesJson.has_value()) {
        mSavedNotesJson = notesToJson(saved.notes);
    }

    const bool finished = saved.resumePoint.empty();

    // A separate node for a partial one, so an older version that knows only TRANSCRIPTION skips it
    // rather than showing it as finished.
    ValueTree tree(finished ? NnId::TranscriptionId : NnId::PartialTranscriptionId);
    tree.setProperty(NnId::TranscriptionFormatVersionId,
                     finished ? TRANSCRIPTION_FORMAT_VERSION : PARTIAL_TRANSCRIPTION_FORMAT_VERSION,
                     nullptr);
    tree.setProperty(NnId::TranscriptionModelSizeId, modelSizeToString(saved.modelSize), nullptr);
    tree.setProperty(NnId::TranscriptionNotesId, *mSavedNotesJson, nullptr);

    if (!finished) {
        tree.setProperty(NnId::FinalizedThroughId, saved.finalizedThrough, nullptr);
        tree.setProperty(NnId::ResumePointId, String(saved.resumePoint), nullptr);
        tree.setProperty(NnId::ResumeProgressId, saved.resumeProgress, nullptr);
    }

    return tree;
}

void TranscriptionManager::restoreFromStateTree(const ValueTree& inFullState)
{
    if (mJobActive.load()) {
        return;
    }

    // Replaced even when the restored state has no transcription: the audio path may not have
    // changed, in which case nothing else would drop the notes on screen.
    if (mProcessor->getState() == PopulatedAudioAndMidiRegions || mProcessor->getState() == Paused) {
        mProcessor->clearTranscription();
    }

    if (mProcessor->getState() != AudioLoaded) {
        return;
    }

    ValueTree tree = inFullState.getChildWithName(NnId::TranscriptionId);
    int format_version = TRANSCRIPTION_FORMAT_VERSION;

    if (!tree.isValid()) {
        tree = inFullState.getChildWithName(NnId::PartialTranscriptionId);
        format_version = PARTIAL_TRANSCRIPTION_FORMAT_VERSION;
    }

    const bool finished = tree.hasType(NnId::TranscriptionId);
    const std::string resume_point = tree.getProperty(NnId::ResumePointId).toString().toStdString();

    if (!tree.isValid() || static_cast<int>(tree.getProperty(NnId::TranscriptionFormatVersionId)) != format_version
        || (!finished && resume_point.empty())) {
        return;
    }

    const double duration = mProcessor->getSourceAudioManager()->getAudioSampleDuration();
    auto notes = parseNotes(tree.getProperty(NnId::TranscriptionNotesId).toString(), duration);

    const std::optional<ModelSize> model_size =
        modelSizeFromString(tree.getProperty(NnId::TranscriptionModelSizeId).toString().toStdString());

    // Resuming with another checkpoint would mix two models' notes. A finished one only shows the name.
    if (!notes.has_value() || (!finished && !model_size.has_value())) {
        return;
    }

    mRawNotes = std::move(*notes);
    mTranscriptionModelSize = model_size.value_or(DEFAULT_MODEL_SIZE);

    if (finished) {
        mFinalizedThrough = duration;
    } else {
        mFinalizedThrough = std::clamp(static_cast<double>(tree.getProperty(NnId::FinalizedThroughId)), 0.0, duration);
        mResumePoint = resume_point;
        mResumeProgress = std::clamp(static_cast<float>(tree.getProperty(NnId::ResumeProgressId)), 0.0f, 1.0f);
    }

    _updateSavedTranscription();

    // Before post-processing, which only runs on a transcription the processor says it has.
    if (finished) {
        mProcessor->setStateToPopulatedAudioAndMidiRegions();
    } else {
        mProcessor->setStateToPaused();
    }

    _updatePostProcessing();
    _repaintPianoRoll();
}

void TranscriptionManager::_updateSavedTranscription()
{
    jassert(MessageManager::getInstance()->isThisTheMessageThread());
    jassert(mTranscriptionModelSize.has_value());

    SavedTranscription saved {mRawNotes, *mTranscriptionModelSize, mFinalizedThrough, mResumePoint, mResumeProgress};

    const ScopedLock sl(mSavedTranscriptionLock);
    mSavedTranscription = std::move(saved);
    mSavedNotesJson.reset();
}

void TranscriptionManager::_repaintPianoRoll()
{
    auto* main_view = mProcessor->getNeuralNoteMainView();

    if (main_view) {
        main_view->repaintPianoRoll();
    }
}
