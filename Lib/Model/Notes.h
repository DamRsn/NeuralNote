//
// Created by Tibor Vass on 04.03.23.
//

#ifndef Notes_h
#define Notes_h

#include <cmath>
#include <json.hpp>
#include <vector>

#include "Constants.h"

enum PitchBendModes
{
    NoPitchBend = 0,
    SinglePitchBend,
    MultiPitchBend
};

NLOHMANN_JSON_SERIALIZE_ENUM(PitchBendModes,
                             {
                                 {NoPitchBend, nullptr},
                                 {SinglePitchBend, "single"},
                                 {MultiPitchBend, "multi"},
                             })

class Notes
{
public:
    typedef struct Event
    {
        double start;
        double end;
        int pitch; // pitch is not in Hz, but in "MIDI note number"
        double amplitude;
        std::vector<int> bends;

        bool operator==(const struct Event&) const;
    } Event;

    typedef struct ConvertParams
    {
        /* Confidence threshold (0.05 to 0.95, More-Less notes) */
        float onsetThreshold = 0.3;
        /* Note segmentation (0.05 - 0.95, Split-Merge Notes) */
        float frameThreshold = 0.5;
        int minNoteLength = 11;
        bool inferOnsets = false;
        float maxFrequency = -1; // in Hz, -1 means unset
        float minFrequency = -1; // in Hz, -1 means unset
        bool melodiaTrick = false;
        enum PitchBendModes pitchBend = NoPitchBend;
        int energyThreshold = 11;
    } ConvertParams;

    // PG stands for posteriorgrams
    std::vector<Notes::Event> convert(const std::vector<std::vector<float>>& inNotesPG,
                                      const std::vector<std::vector<float>>& inOnsetsPG,
                                      const std::vector<std::vector<float>>& inContoursPG,
                                      ConvertParams inParams);

private:
    static inline double _modelFrameToTime(int frame)
    {
        // The following are compile-time computed consts only used here.
        // If they need to be used elsewhere, please move to Constants.h

        static constexpr int ANNOTATIONS_FPS = AUDIO_SAMPLE_RATE / FFT_HOP;
        // number of frames in the time-frequency representations we compute
        static constexpr int ANNOT_N_FRAMES = ANNOTATIONS_FPS * AUDIO_WINDOW_LENGTH;
        // number of samples in the (clipped) audio that we use as input to the models
        static constexpr int AUDIO_N_SAMPLES =
            AUDIO_SAMPLE_RATE * AUDIO_WINDOW_LENGTH - FFT_HOP;
        // magic from Basic Pitch
        static constexpr double WINDOW_OFFSET =
            (double) FFT_HOP / AUDIO_SAMPLE_RATE
                * (ANNOT_N_FRAMES - AUDIO_N_SAMPLES / (double) FFT_HOP)
            + 0.0018;

        return (frame * FFT_HOP) / (double) (AUDIO_SAMPLE_RATE) -WINDOW_OFFSET
               * (frame / ANNOT_N_FRAMES);
    }

    static inline int _hzToFreqIdx(float hz)
    {
        return (int) std::round(12.0 * (std::log2(hz) - std::log2(440.0)) + 69.0
                                - MIDI_OFFSET);
    }

    // _inferredOnsets returns outInferredOnsets a version of inOnsetsPG augmented by detecting
    // differences in note posteriorgrams across frames separated by varying offsets (up to inNumDiffs).
    template <typename T>
    static std::vector<std::vector<T>>
        _inferredOnsets(const std::vector<std::vector<T>>& inOnsetsPG,
                        const std::vector<std::vector<T>>& inNotesPG,
                        int inNumDiffs = 2)
    {
        auto n_frames = inNotesPG.size();
        auto n_notes = inNotesPG[0].size();

        // The algorithm starts by calculating a diff of note posteriorgrams, hence the name notes_diff.
        // This same variable will later morph into the inferred onsets output
        // notes_diff needs to be initialized to all 1 to not interfere with minima
        // calculations, assuming all values in inNotesPG are probabilities < 1.
        auto notes_diff =
            std::vector<std::vector<T>>(n_frames, std::vector<T>(n_notes, 1));

        // max of minima of notes_diff
        T max_min_notes_diff = 0;
        // max of onsets
        T max_onset = 0;

        // for each frame offset
        for (int n = 0; n < inNumDiffs; n++)
        {
            auto offset = n + 1;
            // for each frame
            for (int i = 0; i < n_frames; i++)
            {
                // frame index slided back by offset
                auto i_behind = i - offset;
                // for each note
                for (int j = 0; j < n_notes; j++)
                {
                    // calculate the difference in note probabilities between frame i and
                    // frame i_behind (the frame behind by offset).
                    auto diff =
                        inNotesPG[i][j] - ((i_behind >= 0) ? inNotesPG[i_behind][j] : 0);

                    // Basic Pitch calculates the minimum amongst positive and negative
                    // diffs instead of ignoring negative diffs (which mean "end of note")
                    // while we are only looking for "start of note" (aka onset).
                    // TODO: the zeroing of negative diff should probably happen before
                    // searching for minimum
                    auto& min = notes_diff[i][j];
                    if (diff < min)
                    {
                        diff = (diff < 0) ? 0 : diff;
                        // https://github.com/spotify/basic-pitch/blob/86fc60dab06e3115758eb670c92ead3b62a89b47/basic_pitch/note_creation.py#L298
                        min = (i >= inNumDiffs) ? diff : 0;
                    }

                    // if last diff, max_min_notes_diff can be computed
                    if (offset == inNumDiffs)
                    {
                        auto onset = inOnsetsPG[i][j];
                        if (onset > max_onset)
                        {
                            max_onset = onset;
                        }
                        if (min > max_min_notes_diff)
                        {
                            max_min_notes_diff = min;
                        }
                    }
                }
            }
        }

        // Rescale notes_diff in-place to match scale of original onsets
        // and choose the element-wise max between it and the original onsets.
        // This is where notes_diff morphs truly into the inferred onsets.
        for (int i = 0; i < n_frames; i++)
        {
            for (int j = 0; j < n_notes; j++)
            {
                auto& inferred = notes_diff[i][j];
                inferred = max_onset * inferred / max_min_notes_diff;
                auto orig = inOnsetsPG[i][j];
                if (orig > inferred)
                {
                    inferred = orig;
                }
            }
        }

        return notes_diff;
    }
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    Notes::Event, start, end, pitch, amplitude, bends)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Notes::ConvertParams,
                                                onsetThreshold,
                                                frameThreshold,
                                                minNoteLength,
                                                inferOnsets,
                                                maxFrequency,
                                                minFrequency,
                                                melodiaTrick,
                                                pitchBend,
                                                energyThreshold)

#endif // Notes_h
