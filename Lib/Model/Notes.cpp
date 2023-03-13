//
// Created by Tibor Vass on 04.03.23.
//

#include "Notes.h"

bool Notes::Event::operator==(const Notes::Event& other) const
{
    return this->start == other.start && this->end == other.end
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

    // constrain frequencies
    auto max_note_idx =
        (inParams.maxFrequency < 0) ? n_notes - 1 : _hzToFreqIdx(inParams.maxFrequency);
    auto min_note_idx =
        (inParams.minFrequency < 0) ? 0 : _hzToFreqIdx(inParams.minFrequency);

    // stop 1 frame early to prevent edge case
    // as per https://github.com/spotify/basic-pitch/blob/f85a8e9ade1f297b8adb39b155c483e2312e1aca/basic_pitch/note_creation.py#L399
    int last_frame = n_frames - 1;

    // Go backwards in time
    for (int frame_idx = last_frame - 1; frame_idx >= 0; frame_idx--)
    {
        for (int note_idx = max_note_idx; note_idx >= min_note_idx; note_idx--)
        {
            auto p = onsets[frame_idx][note_idx];

            // equivalent to argrelmax logic
            auto before = (frame_idx <= 0) ? p : onsets[frame_idx - 1][note_idx];
            auto after = (frame_idx >= last_frame) ? p : onsets[frame_idx + 1][note_idx];

            if ((p < inParams.onsetThreshold) || (p < before) || (p < after))
            {
                continue;
            }

            // find time index at this frequency band where the frames drop below an energy threshold
            int i = frame_idx + 1;
            int k = 0; // number of frames since energy dropped below threshold
            while (i < last_frame && k < inParams.energyThreshold)
            {
                if (remaining_energy[i][note_idx] < inParams.frameThreshold)
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
                auto& v = remaining_energy[f];
                amplitude += inNotesPG[f][note_idx]; // could be replaced by v[note_idx]
                v[note_idx] = 0;
                if (note_idx < MAX_NOTE_IDX)
                {
                    v[note_idx + 1] = 0;
                }
                if (note_idx > 0)
                {
                    v[note_idx - 1] = 0;
                }
            }
            amplitude /= (i - frame_idx);

            events.push_back(Notes::Event {
                .start = _modelFrameToTime(frame_idx),
                .end = _modelFrameToTime(i),
                .pitch = note_idx + MIDI_OFFSET,
                .amplitude = amplitude,
            });
        }
    }

    // TODO: melodia trick

    // TODO: pitchbend

    return events;
}
