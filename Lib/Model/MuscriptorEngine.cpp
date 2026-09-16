//
// Created by Damien Ronssin on 03.08.26.
//

#include "MuscriptorEngine.h"

#include <span>

#include <JuceHeader.h>

#include "NNFileUtils.h"
#include "TranscriptionConstants.h"

static_assert(juce::exactlyEqual(TRANSCRIPTION_SAMPLE_RATE, static_cast<double>(msl::Transcriber::SAMPLE_RATE)),
              "TRANSCRIPTION_SAMPLE_RATE must match the sample rate the model expects");

namespace
{
// msl::Note carries no velocity (the model only emits onset/offset/pitch), so a fixed
// synthetic velocity is used here, mirroring the reference implementation's own convention
// of hardcoding a velocity rather than fabricating one that would look like real data.
constexpr double FIXED_AMPLITUDE = 100.0 / 127.0;

NoteEvent toNoteEvent(const msl::Note& inNote)
{
    NoteEvent event;
    event.startTime = inNote.onset;
    event.endTime = inNote.offset;
    event.pitch = inNote.pitch;
    event.amplitude = FIXED_AMPLITUDE;
    event.program = inNote.program;
    return event;
}
} // namespace

void MuscriptorEngine::reset()
{
    {
        const std::lock_guard<std::mutex> lock(mStagingMutex);
        mStaging.clear();
        mStaging.shrink_to_fit();
        mFinalizedThrough = 0.0;
    }

    mFinalNotes.clear();
    // Unlike the clear() in transcribeToMIDI, this one gives the memory back: reset() is also the
    // "no transcription loaded" path, and a full-mix note vector is not small.
    mFinalNotes.shrink_to_fit();
    mLastErrorMessage.clear();
    mCancelRequested = false;
    mProgress = 0.f;
}

bool MuscriptorEngine::_loadModel(ModelSize inModelSize)
{
    jassert(!mTranscriber.has_value());

    const File model_file = NNFileUtils::getModelFile(inModelSize);
    auto loaded = msl::Transcriber::load(NNFileUtils::toPath(model_file), {.use_gpu = true});

    if (!loaded.has_value()) {
        // A checkpoint from another release that happens to have this one's size counts as
        // installed, so this is the first place it shows. Deleting it is what makes it downloadable.
        mLastErrorMessage = loaded.error() == msl::Error::UnsupportedCheckpointVersion
                                ? model_file.getFileName().toStdString()
                                      + " is for another version of NeuralNote. Delete it from the models folder,"
                                        " then download it again"
                                : std::string(msl::describe(loaded.error()));
        Logger::writeToLog("MuscriptorEngine: failed to load " + model_file.getFullPathName() + ": "
                           + String(mLastErrorMessage));
        return false;
    }

    mTranscriber = std::move(*loaded);

    Logger::writeToLog("MuscriptorEngine: model loaded, backend: " + String(mTranscriber->backendName()));

    return true;
}

MuscriptorEngine::Outcome MuscriptorEngine::transcribeToMIDI(ModelSize inModelSize,
                                                             const float* inAudio,
                                                             int inNumSamples,
                                                             const std::vector<msl::InstrumentGroup>& inInstruments)
{
    // mCancelRequested and mProgress are deliberately not touched here. cancel() can be called as
    // soon as the plugin enters the Processing state, which happens before the thread pool gets
    // this far, so clearing the request here would silently drop it. reset() arms both, on the
    // message thread, before the job is queued.
    mFinalNotes.clear();
    mLastErrorMessage.clear();

    if (!_loadModel(inModelSize)) {
        return Outcome::Failed;
    }

    // Unload on every path out, including cancellation and failure: holding a gigabyte of weights
    // and KV cache for a plugin that may not transcribe again for hours is not worth the ~0.3 s a
    // warm reload costs.
    const ScopeGuard unload_model {[this] { mTranscriber.reset(); }};

    // Loading is the one stretch with no callback to observe cancellation from, and it is ~10 s
    // cold. Check on the way out of it rather than making the user wait for the first chunk too.
    if (mCancelRequested.load()) {
        return Outcome::Cancelled;
    }

    const std::span<const float> samples(inAudio, static_cast<size_t>(inNumSamples));

    // Runs on this thread, once per 5 s chunk, with notes that are final and never re-reported.
    // Staged rather than applied: post-processing and the UI belong to the message thread, which
    // drains this on its own timer.
    const msl::NoteCallback callback = [this](const msl::TranscriptionUpdate& inUpdate) {
        {
            const std::lock_guard<std::mutex> lock(mStagingMutex);

            for (const msl::Note& note: inUpdate.new_notes) {
                mStaging.push_back(toNoteEvent(note));
            }

            mFinalizedThrough = inUpdate.finalized_through;
        }

        mProgress = inUpdate.progress;
        return !mCancelRequested.load();
    };

    auto result = mTranscriber->transcribe(samples, {.instruments = inInstruments}, callback);

    if (!result.has_value()) {
        if (result.error() == msl::Error::Cancelled) {
            return Outcome::Cancelled;
        }

        mLastErrorMessage = msl::describe(result.error());
        Logger::writeToLog("MuscriptorEngine: transcription failed: " + String(mLastErrorMessage));
        return Outcome::Failed;
    }

    mFinalNotes.reserve(result->size());

    for (const auto& note: *result) {
        mFinalNotes.push_back(toNoteEvent(note));
    }

    mProgress = 1.f;

    return Outcome::Success;
}

bool MuscriptorEngine::drainNewNotes(std::vector<NoteEvent>& ioNotes, double& ioFinalizedThrough)
{
    const std::lock_guard<std::mutex> lock(mStagingMutex);

    // A chunk can decode no notes and still move the horizon, which is a real change: it is how
    // far the caller may now play, and the frontier it draws.
    if (mStaging.empty() && mFinalizedThrough <= ioFinalizedThrough) {
        return false;
    }

    ioNotes.insert(ioNotes.end(), mStaging.begin(), mStaging.end());
    mStaging.clear();
    ioFinalizedThrough = mFinalizedThrough;

    return true;
}

std::vector<NoteEvent> MuscriptorEngine::takeFinalNotes()
{
    return std::move(mFinalNotes);
}

const std::string& MuscriptorEngine::getLastErrorMessage() const
{
    return mLastErrorMessage;
}

void MuscriptorEngine::cancel()
{
    mCancelRequested = true;
}

float MuscriptorEngine::getProgress() const
{
    return mProgress.load();
}
