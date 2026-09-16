//
// Created by Damien Ronssin on 03.08.26.
//

#ifndef NoteEvent_h
#define NoteEvent_h

#include <algorithm>
#include <tuple>
#include <vector>

#include "muscriptor/muscriptor.hpp"

// 0-127 for a melodic note, msl::DRUM_PROGRAM (128) for a drum hit, so one integer identifies the
// instrument everywhere: the synth's tsf instance, the piano roll's colour, and eventually the MIDI
// track. The model can only reach 35 of those values -- the 34 group representatives plus drums --
// but indexing by program keeps the identity the library gave us instead of inventing a second one.
inline constexpr int NUM_INSTRUMENT_IDS = msl::DRUM_PROGRAM + 1;

struct NoteEvent {
    double startTime = 0.0; // seconds
    double endTime = 0.0; // seconds
    int pitch = 0; // MIDI note number, 0-127
    double amplitude = 0.0; // 0-1, velocity-like value used for synth gain and MIDI velocity
    int program = 0; // 0-127, or msl::DRUM_PROGRAM

    /**
     * A complete test on its own. The library resolves drums before we see them: the note assembler
     * derives is_drum from the instrument group and then overwrites program with DRUM_PROGRAM, so
     * no melodic note carries that program and no drum hit carries any other. That also settles the
     * reference's program-96 quirk, which arrives here already routed as a drum.
     */
    bool isDrum() const { return program == msl::DRUM_PROGRAM; }

    bool operator==(const NoteEvent&) const = default;

    /**
     * Inplace sort of note events by start time.
     * @param inOutEvents
     */
    static inline void sortEvents(std::vector<NoteEvent>& inOutEvents)
    {
        // Ties broken on instrument and pitch as well as end time, so a chord shared by two
        // instruments has one definite order rather than whatever the sort happened to produce.
        std::sort(inOutEvents.begin(), inOutEvents.end(), [](const NoteEvent& a, const NoteEvent& b) {
            return std::tie(a.startTime, a.program, a.pitch, a.endTime)
                   < std::tie(b.startTime, b.program, b.pitch, b.endTime);
        });
    }

    /**
     * Merges note events of the same instrument and pitch that are overlapping in time, so one
     * instrument never gets two overlapping note-on messages for the same pitch (which can otherwise
     * cut a still-sounding note short on the synth). inOutEvents is expected to be sorted
     * (sortEvents is called internally).
     *
     * Keyed on the instrument as well as the pitch: two instruments playing the same note at the
     * same time is ordinary music, and merging those would delete one of them.
     * @param inOutEvents
     */
    static void mergeOverlappingNotesWithSamePitch(std::vector<NoteEvent>& inOutEvents)
    {
        sortEvents(inOutEvents);
        for (size_t i = 0; i + 1 < inOutEvents.size(); i++) {
            auto& event = inOutEvents[i];
            for (size_t j = i + 1; j < inOutEvents.size(); j++) {
                auto& event2 = inOutEvents[j];

                // If notes don't overlap, break
                if (event2.startTime >= event.endTime) {
                    break;
                }

                // If notes overlap and are the same instrument and pitch: merge them
                if (event.pitch == event2.pitch && event.program == event2.program) {
                    event.endTime = std::max(event.endTime, event2.endTime);
                    inOutEvents.erase(inOutEvents.begin() + static_cast<std::ptrdiff_t>(j));
                    j--;
                }
            }
        }
    }
};

#endif // NoteEvent_h
