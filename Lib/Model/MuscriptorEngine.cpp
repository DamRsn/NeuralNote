//
// Created by Damien Ronssin on 03.08.26.
//

#include "MuscriptorEngine.h"

#include <span>

#include <JuceHeader.h>

#include "ComputeDevices.h"
#include "NNFileUtils.h"
#include "TranscriptionConstants.h"

static_assert(juce::exactlyEqual(TRANSCRIPTION_SAMPLE_RATE, static_cast<double>(msl::Transcriber::SAMPLE_RATE)),
              "TRANSCRIPTION_SAMPLE_RATE must match the sample rate the model expects");

namespace
{
NoteEvent toNoteEvent(const msl::Note& inNote)
{
    NoteEvent event;
    event.startTime = inNote.onset;
    event.endTime = inNote.offset;
    event.pitch = inNote.pitch;
    event.amplitude = FIXED_NOTE_AMPLITUDE;
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
        mResumePoint.clear();
    }

    mFinalNotes.clear();
    // Unlike the clear() in transcribeToMIDI, this one gives the memory back: reset() is also the
    // "no transcription loaded" path, and a full-mix note vector is not small.
    mFinalNotes.shrink_to_fit();
    mLastErrorMessage.clear();
    mCancelRequested = false;
    mProgress = Progress {};
}

MuscriptorEngine::Outcome MuscriptorEngine::_loadModel(ModelSize inModelSize, const ComputeDeviceChoice& inDevice)
{
    jassert(!mTranscriber.has_value());

    const File model_file = NNFileUtils::getModelFile(inModelSize);

    // Waits if the editor's listing is still running; it cannot be cancelled, like the GPU
    // initialisation inside load().
    const std::vector<msl::Device>& devices = ComputeDevices::get();

    msl::LoadOptions options;
    options.device = ComputeDevices::resolve(devices, inDevice);
    options.should_cancel = [this] { return mCancelRequested.load(); };
    options.on_progress = [this](float inProgress) { mProgress = Progress {Phase::LoadingModel, inProgress}; };

    auto loaded = msl::Transcriber::load(NNFileUtils::toPath(model_file), std::move(options));

    if (!loaded.has_value()) {
        if (loaded.error() == msl::Error::Cancelled) {
            return Outcome::Cancelled;
        }

        // A checkpoint from another release that happens to have this one's size counts as
        // installed, so this is the first place it shows. Deleting it is what makes it downloadable.
        if (loaded.error() == msl::Error::UnsupportedCheckpointVersion) {
            mLastErrorMessage = model_file.getFileName().toStdString()
                                + " is for another version of NeuralNote. Delete it from the models folder,"
                                  " then download it again";
        }

        // Only an explicit choice can fail this way. The list lasts as long as the plugin binary is
        // loaded, which in a DAW can outlast closing NeuralNote.
        else if (loaded.error() == msl::Error::DeviceUnavailable && options.device.has_value()) {
            mLastErrorMessage = ComputeDevices::label(devices, *options.device)
                                + " could not be used. Choose Auto or CPU under Settings > Compute device,"
                                  " or restart NeuralNote (or your DAW) if your GPUs have changed";
        }

        else {
            mLastErrorMessage = msl::describe(loaded.error());
        }

        Logger::writeToLog("MuscriptorEngine: failed to load " + model_file.getFullPathName() + ": "
                           + String(mLastErrorMessage));
        return Outcome::Failed;
    }

    mTranscriber = std::move(*loaded);

    Logger::writeToLog("MuscriptorEngine: model loaded on " + String(mTranscriber->device().name) + " ("
                       + String(mTranscriber->device().backend) + ")");

    return Outcome::Success;
}

MuscriptorEngine::Outcome MuscriptorEngine::transcribeToMIDI(ModelSize inModelSize,
                                                             const ComputeDeviceChoice& inDevice,
                                                             const float* inAudio,
                                                             int inNumSamples,
                                                             const std::vector<msl::InstrumentGroup>& inInstruments,
                                                             const std::string& inResumeFrom,
                                                             float inStartProgress)
{
    // mCancelRequested and mProgress are deliberately not touched here. cancel() can be called as
    // soon as the plugin enters the Processing state, which happens before the thread pool gets
    // this far, so clearing the request here would silently drop it. reset() arms both, on the
    // message thread, before the job is queued.
    mFinalNotes.clear();
    mLastErrorMessage.clear();

    if (const Outcome loaded = _loadModel(inModelSize, inDevice); loaded != Outcome::Success) {
        return loaded;
    }

    // Unload on every path out, including cancellation and failure: holding a gigabyte of weights
    // and KV cache for a plugin that may not transcribe again for hours is not worth the ~0.3 s a
    // warm reload costs.
    const ScopeGuard unload_model {[this] { mTranscriber.reset(); }};

    mProgress = Progress {Phase::Transcribing, inStartProgress};

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

            // The final update has none: the run is about to succeed, and nothing is left to resume.
            if (!inUpdate.resume_point.empty()) {
                mResumePoint = inUpdate.resume_point;
            }
        }

        mProgress = Progress {Phase::Transcribing, inUpdate.progress};
        return true;
    };

    msl::TranscribeOptions options;
    options.instruments = inInstruments;
    options.should_cancel = [this] { return mCancelRequested.load(); };
    options.resume_from = inResumeFrom;

    auto result = mTranscriber->transcribe(samples, options, callback);

    if (!result.has_value()) {
        if (result.error() == msl::Error::Cancelled) {
            return Outcome::Cancelled;
        }

        if (result.error() == msl::Error::InvalidResumePoint) {
            mLastErrorMessage = msl::describe(result.error());
            Logger::writeToLog("MuscriptorEngine: could not resume: " + String(mLastErrorMessage));
            return Outcome::CannotResume;
        }

        mLastErrorMessage = msl::describe(result.error());
        Logger::writeToLog("MuscriptorEngine: transcription failed: " + String(mLastErrorMessage));
        return Outcome::Failed;
    }

    mFinalNotes.reserve(result->size());

    for (const auto& note: *result) {
        mFinalNotes.push_back(toNoteEvent(note));
    }

    mProgress = Progress {Phase::Transcribing, 1.f};

    return Outcome::Success;
}

bool MuscriptorEngine::drainNewNotes(std::vector<NoteEvent>& ioNotes,
                                     double& ioFinalizedThrough,
                                     std::string& ioResumePoint)
{
    const std::lock_guard<std::mutex> lock(mStagingMutex);

    // A chunk can decode no notes and still move the horizon or the resume point, which is a real
    // change: it is how far the caller may now play, and where it can resume from.
    if (mStaging.empty() && mFinalizedThrough <= ioFinalizedThrough && mResumePoint.empty()) {
        return false;
    }

    ioNotes.insert(ioNotes.end(), mStaging.begin(), mStaging.end());
    mStaging.clear();
    ioFinalizedThrough = std::max(ioFinalizedThrough, mFinalizedThrough);

    if (!mResumePoint.empty()) {
        ioResumePoint = std::move(mResumePoint);
        mResumePoint.clear();
    }

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

bool MuscriptorEngine::isCancelRequested() const
{
    return mCancelRequested.load();
}

MuscriptorEngine::Progress MuscriptorEngine::getProgress() const
{
    return mProgress.load();
}
