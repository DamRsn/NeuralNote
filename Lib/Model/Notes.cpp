//
// Created by Tibor Vass on 04.03.23.
//

#include "Notes.h"

bool Notes::Event::operator==(const Notes::Event& other) const
{
    return this->startTime == other.startTime && this->endTime == other.endTime && this->startFrame == other.startFrame
           && this->endFrame == other.endFrame && this->pitch == other.pitch && this->amplitude == other.amplitude
           && this->bends == other.bends;
}

std::vector<Notes::Event> Notes::convert(const std::vector<std::vector<float>>& inNotesPG,
                                         const std::vector<std::vector<float>>& inOnsetsPG,
                                         const std::vector<std::vector<float>>& inContoursPG,
                                         const ConvertParams& inParams,
                                         bool inNewAudio)
{
    std::vector<Event> events;
    events.reserve(1024);

    const auto n_frames = static_cast<int>(inNotesPG.size());
    if (n_frames == 0) {
        return events;
    }

    const auto n_notes = static_cast<int>(inNotesPG[0].size());
    assert(n_frames == inOnsetsPG.size());
    assert(n_frames == inContoursPG.size());
    assert(n_notes == inOnsetsPG[0].size());
    assert(n_notes == NUM_FREQ_OUT);

    std::vector<std::vector<float>> inferred_onsets;
    auto onsets_ptr = &inOnsetsPG;
    if (inParams.inferOnsets) {
        inferred_onsets = _inferredOnsets<float>(inOnsetsPG, inNotesPG);
        onsets_ptr = &inferred_onsets;
    }
    auto& onsets = *onsets_ptr;

    if (inNewAudio) {
        mRemainingEnergy = inNotesPG;
    } else {
        // Copy without changing the location of the original data
        assert(mRemainingEnergy.size() == n_frames);
        for (size_t f = 0; f < n_frames; f++) {
            assert(inNotesPG[f].size() == NUM_FREQ_OUT);
            assert(mRemainingEnergy[f].size() == NUM_FREQ_OUT);

            std::copy(inNotesPG[f].begin(), inNotesPG[f].end(), mRemainingEnergy[f].begin());
        }
    }

    if (inParams.melodiaTrick) {
        if (inNewAudio) {
            // Fill mRemainingEnergyIndex
            mRemainingEnergyIndex.clear();
            mRemainingEnergyIndex.reserve(static_cast<size_t>(n_frames) * static_cast<size_t>(NUM_FREQ_OUT));

            for (int frame_idx = 0; frame_idx < n_frames; frame_idx++) {
                for (int freq_idx = 0; freq_idx < NUM_FREQ_OUT; freq_idx++) {
                    mRemainingEnergyIndex.push_back(
                        {&mRemainingEnergy[static_cast<size_t>(frame_idx)][static_cast<size_t>(freq_idx)],
                         frame_idx,
                         freq_idx});
                }
            }

            mRemainingEnergyIndex.shrink_to_fit();
        }
    }

    const auto frame_threshold = inParams.frameThreshold;
    // TODO: infer frame_threshold if < 0, can be merged with inferredOnsets.

    // constrain frequencies
    const auto max_note_idx =
        inParams.maxFrequency < 0 ? n_notes - 1 : NoteUtils::hzToMidi(inParams.maxFrequency) - MIDI_OFFSET;
    const auto min_note_idx = inParams.minFrequency < 0 ? 0 : NoteUtils::hzToMidi(inParams.minFrequency) - MIDI_OFFSET;

    // stop 1 frame early to prevent edge case
    // as per https://github.com/spotify/basic-pitch/blob/f85a8e9ade1f297b8adb39b155c483e2312e1aca/basic_pitch/note_creation.py#L399
    const int last_frame = n_frames - 1;

    // Go backwards in time
    for (int frame_idx = last_frame - 1; frame_idx >= 0; frame_idx--) {
        for (int note_idx = max_note_idx; note_idx >= min_note_idx; note_idx--) {
            auto onset = onsets[frame_idx][note_idx];

            // equivalent to argrelmax logic
            auto prev = frame_idx <= 0 ? onset : onsets[frame_idx - 1][note_idx];
            auto next = frame_idx >= last_frame ? onset : onsets[frame_idx + 1][note_idx];

            if (onset < inParams.onsetThreshold || onset < prev || onset < next) {
                continue;
            }

            // find time index at this frequency band where the frames drop below an energy threshold
            int i = frame_idx + 1;
            int k = 0; // number of frames since energy dropped below threshold
            while (i < last_frame && k < inParams.energyThreshold) {
                if (mRemainingEnergy[i][note_idx] < frame_threshold) {
                    k++;
                } else {
                    k = 0;
                }
                i++;
            }

            i -= k; // go back to frame above threshold

            // if the note is too short, skip it
            if (i - frame_idx <= inParams.minNoteLength) {
                continue;
            }

            double amplitude = 0.0;
            for (int f = frame_idx; f < i; f++) {
                amplitude += mRemainingEnergy[f][note_idx];
                mRemainingEnergy[f][note_idx] = 0;

                if (note_idx < MAX_NOTE_IDX) {
                    mRemainingEnergy[f][note_idx + 1] = 0;
                }
                if (note_idx > 0) {
                    mRemainingEnergy[f][note_idx - 1] = 0;
                }
            }

            amplitude /= (i - frame_idx);

            events.push_back(Event {
                _modelFrameToTime(frame_idx) /* startTime */,
                _modelFrameToTime(i) /* endTime */,
                frame_idx /* startFrame */,
                i /* endFrame */,
                note_idx + MIDI_OFFSET /* pitch */,
                amplitude /* amplitude */,
            });
        }
    }

    if (inParams.melodiaTrick) {
        std::sort(mRemainingEnergyIndex.begin(),
                  mRemainingEnergyIndex.end(),
                  [](const _pg_index& a, const _pg_index& b) { return *a.value > *b.value; });

        // loop through each remaining note probability in descending order
        // until reaching frame_threshold.
        for (auto& [energy_ptr, frame_idx, note_idx]: mRemainingEnergyIndex) {
            auto& energy = *energy_ptr;

            // skip those that have already been zeroed
            if (energy == 0.0f) {
                continue;
            }

            if (energy <= frame_threshold) {
                break;
            }
            energy = 0;

            // this inhibit function zeroes out neighbor notes and keeps track (with k)
            // on how many consecutive frames were below frame_threshold.
            auto inhibit = [frame_threshold](std::vector<std::vector<float>>& pg, int frame_i, int note_i, int k) {
                if (pg[frame_i][note_i] < frame_threshold) {
                    k++;
                } else {
                    k = 0;
                }

                pg[frame_i][note_i] = 0;
                if (note_i < MAX_NOTE_IDX) {
                    pg[frame_i][note_i + 1] = 0;
                }
                if (note_i > 0) {
                    pg[frame_i][note_i - 1] = 0;
                }
                return k;
            };

            // forward pass
            int i = frame_idx + 1;
            int k = 0;
            while (i < last_frame && k < inParams.energyThreshold) {
                k = inhibit(mRemainingEnergy, i, note_idx, k);
                i++;
            }

            const auto i_end = i - 1 - k;

            // backward pass
            i = frame_idx - 1;
            k = 0;
            while (i > 0 && k < inParams.energyThreshold) {
                k = inhibit(mRemainingEnergy, i, note_idx, k);
                i--;
            }

            const auto i_start = i + 1 + k;

            // if the note is too short, skip it
            if (i_end - i_start <= inParams.minNoteLength) {
                continue;
            }

            double amplitude = 0.0;
            for (i = i_start; i < i_end; i++) {
                amplitude += inNotesPG[i][note_idx];
            }
            amplitude /= (i_end - i_start);

            events.push_back(Event {
                _modelFrameToTime(i_start /* startTime */),
                _modelFrameToTime(i_end) /* endTime */,
                i_start /* startFrame */,
                i_end /* endFrame */,
                note_idx + MIDI_OFFSET /* pitch */,
                amplitude /* amplitude */,
            });
        }
    }

    sortEvents(events);

    if (inParams.pitchBend != NoPitchBend) {
        _addPitchBends(events, inContoursPG);
        if (inParams.pitchBend == SinglePitchBend) {
            dropOverlappingPitchBends(events);
        }
    }

    return events;
}

void Notes::clear()
{
    mRemainingEnergy.clear();
    mRemainingEnergy.shrink_to_fit();

    mRemainingEnergyIndex.clear();
    mRemainingEnergyIndex.shrink_to_fit();
}

void Notes::_addPitchBends(std::vector<Event>& inOutEvents,
                           const std::vector<std::vector<float>>& inContoursPG,
                           int inNumBinsTolerance)
{
    for (auto& event: inOutEvents) {
        // midi_pitch_to_contour_bin
        int note_idx =
            CONTOURS_BINS_PER_SEMITONE
            * (event.pitch - 69 + 12 * static_cast<int>(std::round(std::log2(440.0f / ANNOTATIONS_BASE_FREQUENCY))));

        static constexpr int N_FREQ_BINS_CONTOURS = NUM_FREQ_OUT * CONTOURS_BINS_PER_SEMITONE;
        int note_start_idx = std::max(note_idx - inNumBinsTolerance, 0);
        int note_end_idx = std::min(N_FREQ_BINS_CONTOURS, note_idx + inNumBinsTolerance + 1);

        const auto gauss_start = static_cast<float>(std::max(0, inNumBinsTolerance - note_idx));
        const auto pb_shift = inNumBinsTolerance - std::max(0, inNumBinsTolerance - note_idx);

        for (int i = event.startFrame; i < event.endFrame; i++) {
            int bend = 0;
            float max = 0;
            for (int j = note_start_idx; j < note_end_idx; j++) {
                int k = j - note_start_idx;
                float x = gauss_start + static_cast<float>(k);
                float n = x - static_cast<float>(inNumBinsTolerance);

                static constexpr float std = 5.0f;

                // Gaussian
                float w = std::exp(-(n * n) / (2.0f * std * std)) * inContoursPG[i][j];

                if (w > max) {
                    bend = k;
                    max = w;
                }
            }
            event.bends.emplace_back(bend - pb_shift);
        }
    }
}