//
// Created by Tibor Vass on 04.03.23.
//

#ifndef Notes_h
#define Notes_h

#include <json.hpp>
#include <vector>

enum PitchBend { NoPitchBend, SinglePitchBend, MultiPitchBend};

NLOHMANN_JSON_SERIALIZE_ENUM(PitchBend, {
    {NoPitchBend, nullptr},
    {SinglePitchBend, "single"},
    {MultiPitchBend, "multi"},
})

typedef struct NoteEvent {
    float start;
    float end;
    int pitch;
    int amplitude;
    std::vector<int> bends;

    bool operator==(const struct NoteEvent&) const = default;
} NoteEvent;


class NotesConverter
{
public:

    typedef struct {
        float onsetThreshold;
        float frameThreshold;
        int minNoteLength;
        bool inferOnsets;
        float maxFrequency;
        float minFrequency;
        bool melodiaTrick;
        enum PitchBend pitchBend;
        int energyThreshold;
    } Params;

    // PG stands for posteriorgrams
    std::vector<NoteEvent> convert(
        const std::vector<float>& inNotesPG,
        const std::vector<float>& inOnsetsPG,
        const std::vector<float>& inContoursPG,
        Params inParams
    );

};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(NoteEvent, start, end, pitch, amplitude, bends)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(NotesConverter::Params, onsetThreshold, frameThreshold, minNoteLength, inferOnsets, maxFrequency, minFrequency, melodiaTrick, pitchBend, energyThreshold)

#endif // Notes_h
