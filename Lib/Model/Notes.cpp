//
// Created by Tibor Vass on 04.03.23.
//

#include "Notes.h"

std::vector<Notes::Event> Notes::convert(
    const std::vector<std::vector<float>>& inNotesPG,
    const std::vector<std::vector<float>>& inOnsetsPG,
    const std::vector<std::vector<float>>& inContoursPG,
    ConvertParams inParams
)
{
    std::vector<Notes::Event> events;

    auto n_frames = inNotesPG.size();
    if (n_frames == 0)
    {
        return events;
    }
    auto n_notes = inNotesPG[0].size();
    assert(n_frames == inOnsetsPG.size());
    assert(n_frames == inContoursPG.size());
    assert(n_notes == inOnsetsPG[0].size());

    // deep copy
    auto remaining_energy = inNotesPG;

    // TODO: constrain frequency
    // TODO: infer onsets

    // stop 1 frame early to prevent edge case
    // as per https://github.com/spotify/basic-pitch/blob/f85a8e9ade1f297b8adb39b155c483e2312e1aca/basic_pitch/note_creation.py#L399
    int last_frame = n_frames - 1;

    // Go backwards in time
    for(int frame_idx = last_frame - 1; frame_idx >= 0; frame_idx--)
    {
        for(int freq_idx = n_notes - 1; freq_idx >= 0; freq_idx--)
        {
            auto p = inOnsetsPG[frame_idx][freq_idx];

            // equivalent to argrelmax logic
            auto before = (frame_idx == 0) ? p : inOnsetsPG[frame_idx-1][freq_idx];
            auto after = inOnsetsPG[frame_idx+1][freq_idx];
            if ((p < inParams.onsetThreshold) || (p < before) || (p < after))
            {
                continue;
            }

            // find time index at this frequency band where the frames drop below an energy threshold
            int i = frame_idx + 1;
            int k = 0; // number of frames since energy dropped below threshold
            while(i < last_frame && k < inParams.energyThreshold)
            {
                if(remaining_energy[i][freq_idx] < inParams.frameThreshold)
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
            if((i - frame_idx) <= inParams.minNoteLength)
            {
                continue;
            }

            double amplitude = 0.0;
            for (int f = frame_idx; f < i; f++)
            {
                auto& v = remaining_energy[f];
                amplitude += inNotesPG[f][freq_idx]; // could be replaced by v[freq_idx]
                v[freq_idx] = 0;
                if(freq_idx < MAX_FREQ_IDX)
                {
                    v[freq_idx+1] = 0;
                }
                if(freq_idx > 0)
                {
                    v[freq_idx-1] = 0;
                }
            }
            amplitude /= (i - frame_idx);

            events.push_back(Notes::Event{
                .start = _model_frame_to_time(frame_idx),
                .end = _model_frame_to_time(i),
                .pitch = freq_idx + MIDI_OFFSET,
                .amplitude = amplitude,
            });
        }
    }

    // TODO: melodia trick

    // TODO: pitchbend

    return events;
}

