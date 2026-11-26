//
// Created by Nathan M on 21.11.26.
//

#include "DettatoEngine.h"

#include <algorithm>
#include <expected>
#include <span>

#include <JuceHeader.h>

#include "ComputeDevices.h"
#include "NNFileUtils.h"
#include "NnLogger.h"

namespace
{
// From here up MuScriptor gives every program a group of its own, and the soundfont a preset.
constexpr int FIRST_UNGROUPED_PROGRAM = 97;
} // namespace

void DettatoEngine::reset()
{
    mFinalNotes.clear();
    mFinalNotes.shrink_to_fit();
    mLastErrorMessage.clear();
    mCancelRequested = false;
    mProgress = Progress {};
}

std::vector<NoteEvent> DettatoEngine::toNoteEvents(const dtt::Transcription& inTranscription)
{
    std::vector<NoteEvent> events;
    events.reserve(inTranscription.notes.size());

    for (const dtt::Note& note: inTranscription.notes) {
        const dtt::Track& track = inTranscription.tracks[static_cast<std::size_t>(note.track)];

        NoteEvent event;
        event.startTime = note.onset;
        event.endTime = note.offset;
        event.pitch = note.pitch;
        event.amplitude = note.velocity / 127.0;
        event.program = track.is_drum ? msl::DRUM_PROGRAM : playableProgram(track.program);
        events.push_back(event);
    }

    return events;
}

int DettatoEngine::playableProgram(int inProgram)
{
    using Group = msl::InstrumentGroup;

    jassert(inProgram >= 0 && inProgram < msl::DRUM_PROGRAM);

    if (inProgram >= FIRST_UNGROUPED_PROGRAM) {
        return inProgram;
    }

    // The programs whose group is not the nearest one below them.
    switch (inProgram) {
        // Tremolo and pizzicato strings, which follow the contrabass. dettato writes its string
        // ensemble as 44.
        case 44:
        case 45:
            return msl::programFor(Group::StringEnsemble);

        // The muted trumpet, which follows the tuba.
        case 59:
            return msl::programFor(Group::Trumpet);

        // dettato's synth effects. Not what msl::instrumentGroupFor answers: the reference gives
        // 96 to its drums, and these are not drums.
        case 96:
            return msl::programFor(Group::SynthPad);

        default:
            break;
    }

    // The program of each of MuScriptor's named groups, ascending. Asked of the library the
    // soundfont is made from rather than listed here. Drums has no program of its own.
    static const std::vector<int> group_programs = [] {
        std::vector<int> programs;

        for (const Group group: msl::allInstrumentGroups()) {
            if (group != Group::Drums) {
                programs.push_back(msl::programFor(group));
            }
        }

        std::sort(programs.begin(), programs.end());
        return programs;
    }();

    // A group's program is the first of its General MIDI family, or of its part of one.
    const auto next = std::upper_bound(group_programs.begin(), group_programs.end(), inProgram);
    return next == group_programs.begin() ? group_programs.front() : *std::prev(next);
}

std::optional<std::size_t> DettatoEngine::findDevice(const std::vector<msl::Device>& inDevices,
                                                     std::size_t inIndex,
                                                     const std::vector<dtt::Device>& inDettatoDevices)
{
    const msl::Device& wanted = inDevices[inIndex];

    const auto is_wanted = [&wanted](const auto& inDevice) {
        return inDevice.backend == wanted.backend && inDevice.name == wanted.name;
    };

    // How many identical devices come before it in its own list, and so in the other one.
    auto num_before =
        std::count_if(inDevices.begin(), inDevices.begin() + static_cast<std::ptrdiff_t>(inIndex), is_wanted);

    for (std::size_t i = 0; i < inDettatoDevices.size(); ++i) {
        if (is_wanted(inDettatoDevices[i]) && num_before-- == 0) {
            return i;
        }
    }

    return std::nullopt;
}

DettatoEngine::Outcome DettatoEngine::_loadModel(const File& inModelFile, const ComputeDeviceChoice& inDevice)
{
    jassert(!mPipeline.has_value());

    // The device was picked from muscriptor.cpp's list. Waits if the editor's listing is still
    // running, which also keeps dettato.cpp from initialising the GPU backends at the same time.
    const std::vector<msl::Device>& devices = ComputeDevices::get();

    const std::optional<std::size_t> device_index = ComputeDevices::resolve(devices, inDevice);

    // What the library has to say about a failure, or about Auto falling back to the CPU.
    std::string detail;

    dtt::LoadOptions options;
    options.should_cancel = [this] { return mCancelRequested.load(); };
    options.on_progress = [this](float inProgress) { mProgress = Progress {Phase::LoadingModel, inProgress}; };
    options.error_detail = &detail;

    if (device_index.has_value()) {
        options.device = findDevice(devices, *device_index, dtt::availableDevices());
    }

    NnLogger::write(NnLogger::Level::Info,
                    "DettatoEngine: loading " + inModelFile.getFileName() + " ("
                        + File::descriptionOfSizeInBytes(inModelFile.getSize()) + ")");

    const double load_start_ms = Time::getMillisecondCounterHiRes();

    // An explicit device dettato.cpp does not list is not loaded anywhere else.
    std::expected<dtt::Pipeline, dtt::Error> loaded = std::unexpected(dtt::Error::DeviceUnavailable);

    if (!device_index.has_value() || options.device.has_value()) {
        loaded = dtt::Pipeline::load(NNFileUtils::toPath(inModelFile), std::move(options));
    } else {
        detail = "dettato.cpp does not list this device";
    }

    const int load_ms = roundToInt(Time::getMillisecondCounterHiRes() - load_start_ms);

    if (!loaded.has_value()) {
        if (loaded.error() == dtt::Error::Cancelled) {
            NnLogger::write(NnLogger::Level::Info, "DettatoEngine: model loading cancelled");
            return Outcome::Cancelled;
        }

        // As in MuscriptorEngine: a checkpoint from another release that happens to have this
        // one's size counts as installed, so this is the first place it shows.
        if (loaded.error() == dtt::Error::UnsupportedCheckpointVersion) {
            mLastErrorMessage = inModelFile.getFileName().toStdString()
                                + " is for another version of NeuralNote. Delete it from the models folder,"
                                  " then download it again";
        }

        // Under Auto the library has already tried the CPU, which falls to the generic message.
        else if (loaded.error() == dtt::Error::DeviceUnavailable && device_index.has_value()) {
            mLastErrorMessage = ComputeDevices::label(devices, *device_index)
                                + " could not be used. Choose Auto or CPU under Settings > Compute device,"
                                  " or restart NeuralNote (or your DAW) if your GPUs have changed";
        }

        else {
            mLastErrorMessage = dtt::describe(loaded.error());
        }

        NnLogger::write(NnLogger::Level::Error,
                        "DettatoEngine: failed to load " + inModelFile.getFullPathName() + ": "
                            + String(mLastErrorMessage) + (detail.empty() ? String() : " (" + String(detail) + ")"));
        return Outcome::Failed;
    }

    mPipeline = std::move(*loaded);

    NnLogger::write(NnLogger::Level::Info,
                    "DettatoEngine: model loaded on " + String(mPipeline->device().name) + " ("
                        + String(mPipeline->device().backend) + ") in " + String(load_ms) + " ms");

    // Auto ran into a GPU it could not use.
    if (!detail.empty()) {
        NnLogger::write(NnLogger::Level::Warning, "DettatoEngine: " + String(detail));
    }

    return Outcome::Success;
}

DettatoEngine::Outcome DettatoEngine::transcribeToMIDI(const File& inModelFile,
                                                       const ComputeDeviceChoice& inDevice,
                                                       const float* inAudio,
                                                       int inNumSamples,
                                                       double inSampleRate)
{
    // mCancelRequested and mProgress are deliberately not touched here, as in MuscriptorEngine:
    // cancel() can be called before the thread pool gets this far, and reset() is what arms both.
    mFinalNotes.clear();
    mLastErrorMessage.clear();

    if (const Outcome loaded = _loadModel(inModelFile, inDevice); loaded != Outcome::Success) {
        return loaded;
    }

    // Unload on every path out, including cancellation and failure.
    const ScopeGuard unload_model {[this] {
        mPipeline.reset();
        NnLogger::write(NnLogger::Level::Info, "DettatoEngine: model unloaded");
    }};

    mProgress = Progress {Phase::Transcribing, 0.0f};

    const std::span<const float> samples(inAudio, static_cast<size_t>(inNumSamples));

    // Polled between the library's windows of audio and between its stages, and on the CPU at
    // every node of a graph. The progress is over the whole run, each stage weighted by its usual
    // share of the time.
    dtt::TranscribeOptions options;
    options.should_cancel = [this] { return mCancelRequested.load(); };
    options.on_progress = [this](const dtt::Progress& inProgress) {
        mProgress = Progress {Phase::Transcribing, inProgress.overall};
    };

    const double audio_seconds = inNumSamples / inSampleRate;

    const double transcribe_start_ms = Time::getMillisecondCounterHiRes();

    auto result = mPipeline->transcribe(samples, inSampleRate, options);

    const double wall_seconds = (Time::getMillisecondCounterHiRes() - transcribe_start_ms) / 1000.0;

    if (!result.has_value()) {
        if (result.error() == dtt::Error::Cancelled) {
            NnLogger::write(NnLogger::Level::Info,
                            "DettatoEngine: transcription cancelled after " + String(wall_seconds, 1) + " s");
            return Outcome::Cancelled;
        }

        // The library keeps the reason for an argument it rejected and for an internal error.
        const std::string& reason = mPipeline->lastErrorMessage();
        mLastErrorMessage = reason.empty() ? dtt::describe(result.error()) : reason;
        NnLogger::write(NnLogger::Level::Error,
                        "DettatoEngine: transcription failed after " + String(wall_seconds, 1)
                            + " s: " + String(mLastErrorMessage));
        return Outcome::Failed;
    }

    NnLogger::write(
        NnLogger::Level::Info,
        "DettatoEngine: transcribed " + String(result->notes.size()) + " notes on " + String(result->tracks.size())
            + " tracks in " + String(wall_seconds, 1) + " s, "
            + String(wall_seconds > 0.0 ? audio_seconds / wall_seconds : 0.0, 2) + "x real time"
            + (result->drum_model.empty() ? String(", no drums") : ", drums: " + String(result->drum_model)));

    mFinalNotes = toNoteEvents(*result);

    mProgress = Progress {Phase::Transcribing, 1.f};

    return Outcome::Success;
}

std::vector<NoteEvent> DettatoEngine::takeFinalNotes()
{
    return std::move(mFinalNotes);
}

const std::string& DettatoEngine::getLastErrorMessage() const
{
    return mLastErrorMessage;
}

void DettatoEngine::cancel()
{
    mCancelRequested = true;
}

bool DettatoEngine::isCancelRequested() const
{
    return mCancelRequested.load();
}

DettatoEngine::Progress DettatoEngine::getProgress() const
{
    return mProgress.load();
}
