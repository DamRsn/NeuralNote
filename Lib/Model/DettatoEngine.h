//
// Created by Nathan M on 21.11.26.
//

#ifndef DettatoEngine_h
#define DettatoEngine_h

#include <atomic>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "dettato/dettato.hpp"
#include "muscriptor/device.hpp"

#include "MuscriptorEngine.h"
#include "NnGlobalSettings.h"
#include "NoteEvent.h"

/**
 * Wraps dtt::Pipeline, dettato.cpp's transcriber, with the shape of MuscriptorEngine: a blocking
 * transcription that ends in the same Outcome, its notes converted to NoteEvent.
 *
 * What dettato does differently is left as it is rather than worked around:
 *
 * - Nothing streams and nothing can be resumed. dettato assigns its notes to instrument tracks
 *   only once it has all of them, so they arrive together when the run succeeds. There is no
 *   drainNewNotes, and a cancelled run leaves nothing behind.
 * - There is no instrument conditioning: the model names the instruments it hears.
 * - The audio is mono at the rate of its source, anything from 16 kHz up, rather than at
 *   TRANSCRIPTION_SAMPLE_RATE. The drum model reads the 7.8-20 kHz band, so drums are only
 *   transcribed from audio at 40 kHz or more; below that the result is the pitched notes alone.
 * - Notes carry a velocity: the drum model's for a drum hit, and for a pitched note, whose
 *   loudness dettato does not predict, the model's confidence in it.
 *
 * Its programs are the exception. dettato names 73 instruments by their General MIDI program,
 * and the synth's soundfont, the instrument names and the colours only know the one program
 * MuScriptor emits for each of its instrument groups. A track's program is folded onto its
 * group's, see playableProgram, so two dettato tracks of one group, a flute and a piccolo,
 * become one instrument.
 *
 * As with MuscriptorEngine, the model does not outlive the transcription that loaded it. The
 * checkpoint is 74 MB, and loading it is a small part of any run.
 */
class DettatoEngine
{
public:
    // MuscriptorEngine's own, so that whoever runs either engine holds one kind of result.
    // CannotResume is never returned: there is no resume point to reject.
    using Outcome = MuscriptorEngine::Outcome;
    using Phase = MuscriptorEngine::Phase;
    using Progress = MuscriptorEngine::Progress;

    DettatoEngine() = default;

    /**
     * Drops what the last transcription produced and arms the engine for a new one, as
     * MuscriptorEngine::reset: pending cancellation and progress are reset here and nowhere else.
     * Must be called before every transcribeToMIDI call, while no job is in flight.
     */
    void reset();

    /**
     * Transcribes the input audio. Blocking, can take from seconds to minutes: must be called
     * off the message/audio thread, and only after reset(). Loads the model on entry and unloads
     * it before returning, whatever the outcome.
     *
     * The notes are the model's own output: nothing is merged, filtered or otherwise
     * post-processed here.
     * @param inModelFile The checkpoint to load, getDettatoManifestEntry()'s file.
     * @param inDevice Where to run it. An explicit device that cannot be used fails the call rather
     *        than running elsewhere; Auto, which a device that is no longer listed resolves to,
     *        falls back to the CPU.
     * @param inAudio Pointer to raw audio (must be mono at inSampleRate Hz).
     * @param inNumSamples Number of input samples available.
     * @param inSampleRate The rate of the audio, which the library resamples itself. At least
     *        16 kHz, and at least 40 kHz for the drums to be transcribed.
     * @return Success only if the final note event vector is usable.
     */
    Outcome transcribeToMIDI(const juce::File& inModelFile,
                             const ComputeDeviceChoice& inDevice,
                             const float* inAudio,
                             int inNumSamples,
                             double inSampleRate);

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

    /** @return Whether cancel() has been called since the last reset(). Thread-safe. */
    bool isCancelRequested() const;

    /** @return Progress of the current/last transcribeToMIDI call. Thread-safe. */
    Progress getProgress() const;

    /**
     * @return inTranscription's notes, in its order. A note takes its track's program as
     *         playableProgram folds it, or msl::DRUM_PROGRAM on the drum track, and its velocity
     *         as an amplitude in [0, 1].
     */
    static std::vector<NoteEvent> toNoteEvents(const dtt::Transcription& inTranscription);

    /**
     * @param inProgram A General MIDI program, 0-127.
     * @return The program the plugin plays it with: the one MuScriptor emits for the instrument
     *         group it belongs to, which is the one the soundfont has a preset for and the one
     *         that names and colours an instrument. From 97 up every program is a group of its
     *         own, and is returned as it is.
     */
    static int playableProgram(int inProgram);

    /**
     * Finds a device of NeuralNote's list, which is muscriptor.cpp's, in dettato.cpp's. The two
     * libraries list their devices separately, so the same GPU is not at the same index in both:
     * it is matched on its backend and its name, and among identical GPUs on its place.
     *
     * @return The index in inDettatoDevices of inDevices[inIndex], or nullopt if it is not there.
     */
    static std::optional<std::size_t> findDevice(const std::vector<msl::Device>& inDevices,
                                                 std::size_t inIndex,
                                                 const std::vector<dtt::Device>& inDettatoDevices);

private:
    /** @return Success with mPipeline set, or why not; Failed also sets mLastErrorMessage. */
    Outcome _loadModel(const juce::File& inModelFile, const ComputeDeviceChoice& inDevice);

    // Only holds a value for the duration of a transcribeToMIDI call.
    std::optional<dtt::Pipeline> mPipeline;

    // The whole transcription. Written before the outcome is published, so whoever observes that
    // outcome can read it without further synchronisation.
    std::vector<NoteEvent> mFinalNotes;

    std::string mLastErrorMessage;

    std::atomic<bool> mCancelRequested {false};
    std::atomic<Progress> mProgress {Progress {}};
};

#endif // DettatoEngine_h
