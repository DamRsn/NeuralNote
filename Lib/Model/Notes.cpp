//
// Created by Tibor Vass on 04.03.23.
//

#include "Notes.h"

bool Notes::Event::operator==(const Notes::Event& other) const
{
    return this->startTime == other.startTime && this->endTime == other.endTime
           && this->startFrame == other.startFrame && this->endFrame == other.endFrame
           && this->pitch == other.pitch && this->amplitude == other.amplitude
           && this->bends == other.bends;
}

std::vector<Notes::Event>
    Notes::convert(const std::vector<std::vector<float>>& inNotesPG,
                   const std::vector<std::vector<float>>& inOnsetsPG,
                   const std::vector<std::vector<float>>& inContoursPG,
                   ConvertParams inParams)
{
    std::vector<Notes::Event> events;
    events.reserve(1000);

    auto n_frames = inNotesPG.size();
    if (n_frames == 0)
    {
        return events;
    }

    auto n_notes = inNotesPG[0].size();
    assert(n_frames == inOnsetsPG.size());
    assert(n_frames == inContoursPG.size());
    assert(n_notes == inOnsetsPG[0].size());

    std::vector<std::vector<float>> inferred_onsets;
    auto onsets_ptr = &inOnsetsPG;
    if (inParams.inferOnsets)
    {
        inferred_onsets = _inferredOnsets<float>(inOnsetsPG, inNotesPG);
        onsets_ptr = &inferred_onsets;
    }
    auto& onsets = *onsets_ptr;

    // deep copy
    auto remaining_energy = inNotesPG;

    // to-be-sorted index of remaining_energy
    std::vector<_pg_index> remaining_energy_index;
    if (inParams.melodiaTrick)
    {
        remaining_energy_index.reserve(n_frames * n_notes);
    }

    auto frame_threshold = inParams.frameThreshold;
    // TODO: infer frame_threshold if < 0, can be merged with inferredOnsets.

    // constrain frequencies
    auto max_note_idx = (inParams.maxFrequency < 0)
                            ? n_notes - 1
                            : ((_hzToMidi(inParams.maxFrequency) - MIDI_OFFSET));
    auto min_note_idx = (inParams.minFrequency < 0)
                            ? 0
                            : ((_hzToMidi(inParams.minFrequency) - MIDI_OFFSET));

    // stop 1 frame early to prevent edge case
    // as per https://github.com/spotify/basic-pitch/blob/f85a8e9ade1f297b8adb39b155c483e2312e1aca/basic_pitch/note_creation.py#L399
    int last_frame = n_frames - 1;

    // Go backwards in time
    for (int frame_idx = last_frame - 1; frame_idx >= 0; frame_idx--)
    {
        for (int note_idx = max_note_idx; note_idx >= min_note_idx; note_idx--)
        {
            auto onset = onsets[frame_idx][note_idx];

            if (inParams.melodiaTrick)
            {
                remaining_energy_index.emplace_back(_pg_index {
                    &remaining_energy[frame_idx][note_idx], frame_idx, note_idx});
            }

            // equivalent to argrelmax logic
            auto prev = (frame_idx <= 0) ? onset : onsets[frame_idx - 1][note_idx];
            auto next =
                (frame_idx >= last_frame) ? onset : onsets[frame_idx + 1][note_idx];

            if ((onset < inParams.onsetThreshold) || (onset < prev) || (onset < next))
            {
                continue;
            }

            // find time index at this frequency band where the frames drop below an energy threshold
            int i = frame_idx + 1;
            int k = 0; // number of frames since energy dropped below threshold
            while (i < last_frame && k < inParams.energyThreshold)
            {
                if (remaining_energy[i][note_idx] < frame_threshold)
                {
                    k++;
                }
                else
                {
                    k = 0;
                }
                i++;
            }

            i -= k; // go back to frame above threshold

            // if the note is too short, skip it
            if ((i - frame_idx) <= inParams.minNoteLength)
            {
                continue;
            }

            double amplitude = 0.0;
            for (int f = frame_idx; f < i; f++)
            {
                amplitude += remaining_energy[f][note_idx];
                remaining_energy[f][note_idx] = 0;

                if (note_idx < MAX_NOTE_IDX)
                {
                    remaining_energy[f][note_idx + 1] = 0;
                }
                if (note_idx > 0)
                {
                    remaining_energy[f][note_idx - 1] = 0;
                }
            }
            amplitude /= (i - frame_idx);

            events.push_back(Notes::Event {
                _modelFrameToTime(frame_idx) /* startTime */,
                _modelFrameToTime(i) /* endTime */,
                frame_idx /* startFrame */,
                i /* endFrame */,
                note_idx + MIDI_OFFSET /* pitch */,
                amplitude /* amplitude */,
            });
        }
    }

    if (inParams.melodiaTrick)
    {
        std::sort(remaining_energy_index.begin(),
                  remaining_energy_index.end(),
                  [](const _pg_index& a, const _pg_index& b)
                  { return *a.value > *b.value; });

        // loop through each remaining note probability in descending order
        // until reaching frame_threshold.
        for (int r = 0; r < remaining_energy_index.size(); r++)
        {
            auto rei = remaining_energy_index[r];
            auto& frame_idx = rei.frameIdx;
            auto& note_idx = rei.noteIdx;
            auto& energy = *rei.value;

            // skip those that have already been zeroed
            if (energy == 0)
            {
                continue;
            }

            if (energy <= frame_threshold)
            {
                break;
            }
            energy = 0;

            // this inhibit function zeroes out neighbor notes and keeps track (with k)
            // on how many consecutive frames were below frame_threshold.
            auto inhibit = [](std::vector<std::vector<float>>& pg,
                              int frame_idx,
                              int note_idx,
                              float frame_threshold,
                              int k)
            {
                if (pg[frame_idx][note_idx] < frame_threshold)
                {
                    k++;
                }
                else
                {
                    k = 0;
                }

                pg[frame_idx][note_idx] = 0;
                if (note_idx < MAX_NOTE_IDX)
                {
                    pg[frame_idx][note_idx + 1] = 0;
                }
                if (note_idx > 0)
                {
                    pg[frame_idx][note_idx - 1] = 0;
                }
                return k;
            };

            // forward pass
            int i = frame_idx + 1;
            int k = 0;
            while (i < last_frame && k < inParams.energyThreshold)
            {
                k = inhibit(remaining_energy, i, note_idx, frame_threshold, k);
                i++;
            }

            auto i_end = i - 1 - k;

            // backward pass
            i = frame_idx - 1;
            k = 0;
            while (i > 0 && k < inParams.energyThreshold)
            {
                k = inhibit(remaining_energy, i, note_idx, frame_threshold, k);
                i--;
            }

            auto i_start = i + 1 + k;

            // if the note is too short, skip it
            if (i_end - i_start <= inParams.minNoteLength)
            {
                continue;
            }

            double amplitude = 0.0;
            for (int i = i_start; i < i_end; i++)
            {
                amplitude += inNotesPG[i][note_idx];
            }
            amplitude /= (i_end - i_start);

            events.push_back(Notes::Event {
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

    if (inParams.pitchBend != NoPitchBend)
    {
        _addPitchBends(events, inContoursPG);
        if (inParams.pitchBend == SinglePitchBend)
        {
            dropOverlappingPitchBends(events);
        }
    }

    return events;
}

void Notes::_addPitchBends(std::vector<Notes::Event>& inOutEvents,
                           const std::vector<std::vector<float>>& inContoursPG,
                           int inNumBinsTolerance)
{
    auto window_length = inNumBinsTolerance * 2 + 1;
    for (auto& event: inOutEvents)
    {
        // midi_pitch_to_contour_bin
        int note_idx =
            CONTOURS_BINS_PER_SEMITONE
            * (event.pitch - 69
               + 12 * std::round(std::log2(440.0 / ANNOTATIONS_BASE_FREQUENCY)));

        static constexpr int N_FREQ_BINS_CONTOURS =
            NUM_FREQ_OUT * CONTOURS_BINS_PER_SEMITONE;
        int note_start_idx = std::max(note_idx - inNumBinsTolerance, 0);
        int note_end_idx =
            std::min(N_FREQ_BINS_CONTOURS, note_idx + inNumBinsTolerance + 1);

        int gauss_start = std::max(0, inNumBinsTolerance - note_idx);
        auto pb_shift = inNumBinsTolerance - std::max(0, inNumBinsTolerance - note_idx);

        for (int i = event.startFrame; i < event.endFrame; i++)
        {
            int bend = 0;
            float max = 0;
            for (int j = note_start_idx; j < note_end_idx; j++)
            {
                int k = j - note_start_idx;
                float x = gauss_start + k;
                float n = x - inNumBinsTolerance;
                static constexpr float std = 5.0;
                // Gaussian
                float w = std::exp(-(n * n) / (2.0 * std * std));
                w *= inContoursPG[i][j];
                if (w > max)
                {
                    bend = k;
                    max = w;
                }
            }
            event.bends.emplace_back(bend - pb_shift);
        }
    }
}