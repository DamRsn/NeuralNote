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

class Notes
{
public:
    typedef struct Event {
        float start;
        float end;
        int pitch;
        int amplitude;
        std::vector<int> bends;

        bool operator==(const struct Event&) const = default;
    } Event;

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
    } ConvertParams;

    // PG stands for posteriorgrams
    std::vector<Notes::Event> convert(
        const std::vector<float>& inNotesPG,
        const std::vector<float>& inOnsetsPG,
        const std::vector<float>& inContoursPG,
        ConvertParams inParams
    );

};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Notes::Event, start, end, pitch, amplitude, bends)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Notes::ConvertParams, onsetThreshold, frameThreshold, minNoteLength, inferOnsets, maxFrequency, minFrequency, melodiaTrick, pitchBend, energyThreshold)

#endif // Notes_h
