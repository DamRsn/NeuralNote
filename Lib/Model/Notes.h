//
// Created by Tibor Vass on 04.03.23.
//

#ifndef Notes_h
#define Notes_h

#include <cassert>
#include <cmath>
#include <vector>

#include "BasicPitchConstants.h"
#include "NoteUtils.h"

enum PitchBendModes { NoPitchBend = 0, SinglePitchBend, MultiPitchBend };

/**
 * Class to extract note events from posteriorgrams (outputs of basic pitch cnn).
 */
class Notes
{
public:
    typedef struct Event {
        double startTime;
        double endTime;
        int startFrame;
        int endFrame;
        int pitch; // pitch is not in Hz, but in "MIDI note number"
        double amplitude;
        std::vector<int> bends; // One vale of pitch bend per frame. Units is 1/3 of semitones.

        bool operator==(const struct Event&) const;
    } Event;

    typedef struct ConvertParams {
        /* Note segmentation (0.05 - 0.95, Split-Merge Notes) */
        float onsetThreshold = 0.3;
        /* Confidence threshold (0.05 to 0.95, More-Less notes) */
        float frameThreshold = 0.5;
        /* Minimum note length in number of frames */
        int minNoteLength = 11;
        bool inferOnsets = true;
        float maxFrequency = -1; // in Hz, -1 means unset
        float minFrequency = -1; // in Hz, -1 means unset
        bool melodiaTrick = true;
        PitchBendModes pitchBend = NoPitchBend;
        int energyThreshold = 11;
    } ConvertParams;

    /**
     * Create note events based on postegriorgram inputs
     * @param inNotesPG Note posteriorgrams
     * @param inOnsetsPG Onset posteriorgrams
     * @param inContoursPG Contour posteriorgrams
     * @param inParams input parameters
     * @param inNewAudio True: first time calling this function with this audio (these inNotesPG, inOnsetsPG, inContoursPG).
     *  False if same audio as last time with updated parameters.
     * @return
     */
    std::vector<Event> convert(const std::vector<std::vector<float>>& inNotesPG,
                               const std::vector<std::vector<float>>& inOnsetsPG,
                               const std::vector<std::vector<float>>& inContoursPG,
                               const ConvertParams& inParams,
                               bool inNewAudio);

    /**
     * Release any memory allocated by the class.
     */
    void clear();

    /**
     * Inplace sort of note events.
     * @param inOutEvents
     */
    static inline void sortEvents(std::vector<Notes::Event>& inOutEvents)
    {
        std::sort(inOutEvents.begin(), inOutEvents.end(), [](const Event& a, const Event& b) {
            return a.startFrame < b.startFrame || (a.startFrame == b.startFrame && a.endFrame < b.endFrame);
        });
    }

    /**
     * dropOverlappingPitchBends sets bends to an empty array to all the note events that are overlapping in time.
     * inOutEvents is expected to be sorted.
     * @param inOutEvents
     */
    static void dropOverlappingPitchBends(std::vector<Notes::Event>& inOutEvents)
    {
        for (int i = 0; i < int(inOutEvents.size()) - 1; i++) {
            auto& event = inOutEvents[i];
            // if there is an overlap between events, remove pitch bends
            for (int j = i + 1; j < inOutEvents.size(); j++) {
                auto& event2 = inOutEvents[j];
                if (event2.startFrame >= event.endFrame) {
                    break;
                }
                event.bends = std::vector<int>();
                event2.bends = std::vector<int>();
            }
        }
    }

    /**
     * mergeOverlappingNotes merges note events of same pitch that are overlapping in time.
     * inOutEvents is expected to be sorted.
     * @param inOutEvents
     */
    static void mergeOverlappingNotesWithSamePitch(std::vector<Notes::Event>& inOutEvents)
    {
        sortEvents(inOutEvents);
        for (int i = 0; i < int(inOutEvents.size()) - 1; i++) {
            auto& event = inOutEvents[i];
            for (auto j = i + 1; j < inOutEvents.size(); j++) {
                auto& event2 = inOutEvents[j];

                // If notes don't overlap, break
                if (event2.startFrame >= event.endFrame) {
                    break;
                }

                // If notes overlap and have the same pitch: merge them
                if (event.pitch == event2.pitch) {
                    event.endTime = event2.endTime;
                    event.endFrame = event2.endFrame;
                    inOutEvents.erase(inOutEvents.begin() + j);
                }
            }
        }
    }

private:
    /**
     * Add pitch bend vector to note events.
     * @param inOutEvents event vector (input and output)
     * @param inContoursPG Contour posteriorgram matrix
     * @param inNumBinsTolerance
     */
    static void _addPitchBends(std::vector<Notes::Event>& inOutEvents,
                               const std::vector<std::vector<float>>& inContoursPG,
                               int inNumBinsTolerance = 25);

    /**
     * Get time in seconds given frame index.
     * Different behaviour in test because of weirdness in basic-pitch code
     * @param frame Index of frame.
     * @return Corresponding time in seconds.
     */
    static inline double _modelFrameToTime(int frame)
    {
        // The following are compile-time computed consts only used here.
        // If they need to be used elsewhere, please move to Constants.h

        static constexpr int ANNOTATIONS_FPS = AUDIO_SAMPLE_RATE / FFT_HOP;
        // number of frames in the time-frequency representations we compute
        static constexpr int ANNOT_N_FRAMES = ANNOTATIONS_FPS * AUDIO_WINDOW_LENGTH;
        // number of samples in the (clipped) audio that we use as input to the models
        static constexpr int AUDIO_N_SAMPLES = AUDIO_SAMPLE_RATE * AUDIO_WINDOW_LENGTH - FFT_HOP;
        // magic from Basic Pitch
        static constexpr double WINDOW_OFFSET =
            (double) FFT_HOP / AUDIO_SAMPLE_RATE * (ANNOT_N_FRAMES - AUDIO_N_SAMPLES / (double) FFT_HOP) + 0.0018;

        // Weird stuff from Basic Pitch. Use only in test so they can pass.
#if USE_TEST_NOTE_FRAME_TO_TIME
        return (frame * FFT_HOP) / (double) (AUDIO_SAMPLE_RATE) -WINDOW_OFFSET * (frame / ANNOT_N_FRAMES);
#else
        return (frame * FFT_HOP) / (double) (AUDIO_SAMPLE_RATE);
#endif
    }

    /**
     * Returns a version of inOnsetsPG augmented by detecting differences in note posteriorgrams
     * across frames separated by varying offsets (up to inNumDiffs).
     * @tparam T
     * @param inOnsetsPG Onset posteriorgrams
     * @param inNotesPG Note posteriorgrams
     * @param inNumDiffs max varying offset.
     * @return
     */
    // TODO: change to float
    template <typename T>
    static std::vector<std::vector<T>> _inferredOnsets(const std::vector<std::vector<T>>& inOnsetsPG,
                                                       const std::vector<std::vector<T>>& inNotesPG,
                                                       int inNumDiffs = 2)
    {
        auto n_frames = inNotesPG.size();
        auto n_notes = inNotesPG[0].size();

        // The algorithm starts by calculating a diff of note posteriorgrams, hence the name notes_diff.
        // This same variable will later morph into the inferred onsets output
        // notes_diff needs to be initialized to all 1 to not interfere with minima
        // calculations, assuming all values in inNotesPG are probabilities < 1.
        auto notes_diff = std::vector<std::vector<T>>(n_frames, std::vector<T>(n_notes, 1));

        // max of minima of notes_diff
        T max_min_notes_diff = 0;
        // max of onsets
        T max_onset = 0;

        // for each frame offset
        for (int n = 0; n < inNumDiffs; n++) {
            auto offset = n + 1;
            // for each frame
            for (int i = 0; i < n_frames; i++) {
                // frame index slided back by offset
                auto i_behind = i - offset;
                // for each note
                for (int j = 0; j < n_notes; j++) {
                    // calculate the difference in note probabilities between frame i and
                    // frame i_behind (the frame behind by offset).
                    auto diff = inNotesPG[i][j] - ((i_behind >= 0) ? inNotesPG[i_behind][j] : 0);

                    // Basic Pitch calculates the minimum amongst positive and negative
                    // diffs instead of ignoring negative diffs (which mean "end of note")
                    // while we are only looking for "start of note" (aka onset).
                    // TODO: the zeroing of negative diff should probably happen before
                    // searching for minimum
                    auto& min = notes_diff[i][j];
                    if (diff < min) {
                        diff = (diff < 0) ? 0 : diff;
                        // https://github.com/spotify/basic-pitch/blob/86fc60dab06e3115758eb670c92ead3b62a89b47/basic_pitch/note_creation.py#L298
                        min = (i >= inNumDiffs) ? diff : 0;
                    }

                    // if last diff, max_min_notes_diff can be computed
                    if (offset == inNumDiffs) {
                        auto onset = inOnsetsPG[i][j];
                        if (onset > max_onset) {
                            max_onset = onset;
                        }
                        if (min > max_min_notes_diff) {
                            max_min_notes_diff = min;
                        }
                    }
                }
            }
        }

        // Rescale notes_diff in-place to match scale of original onsets
        // and choose the element-wise max between it and the original onsets.
        // This is where notes_diff morphs truly into the inferred onsets.
        for (int i = 0; i < n_frames; i++) {
            for (int j = 0; j < n_notes; j++) {
                auto& inferred = notes_diff[i][j];
                inferred = max_onset * inferred / max_min_notes_diff;
                auto orig = inOnsetsPG[i][j];
                if (orig > inferred) {
                    inferred = orig;
                }
            }
        }

        return notes_diff;
    }

    struct _pg_index {
        float* value;
        int frameIdx;
        int noteIdx;
    };

    std::vector<std::vector<float>> mRemainingEnergy;
    std::vector<_pg_index> mRemainingEnergyIndex;
};

#endif // Notes_h
