//
// Created by Damien Ronssin on 08.08.26.
//

#ifndef NN_PIANOROLLRANGE_H
#define NN_PIANOROLLRANGE_H

#include <algorithm>

#include "TranscriptionConstants.h"

namespace PianoRollRange
{
/** A range of MIDI notes to show, inclusive at both ends. */
struct DisplayRange {
    int lowNote = MIN_MIDI_NOTE;
    int highNote = MAX_MIDI_NOTE;

    bool operator==(const DisplayRange&) const = default;
};

/** C0 through B5, the span an empty piano roll starts from before it is widened to fill. */
inline constexpr int DEFAULT_LOW_OCTAVE = 1;
inline constexpr int DEFAULT_HIGH_OCTAVE = 5;

/** The octave a MIDI note falls in, 0 for notes 0-11. Not the octave in its name. */
inline constexpr int octaveOf(int inNote)
{
    return inNote / 12;
}

/** The highest octave with any note in it. Its top note, 131, does not exist. */
inline constexpr int LAST_OCTAVE = MAX_MIDI_NOTE / 12;

/**
 * Where a white key sits within its octave, in key widths. Mirrors what
 * KeyboardComponentBase::getKeyPosition computes, restricted to white keys -- which is all this
 * needs, because the ranges below always start on a C and end on a white key, and the black keys'
 * own positions never move an endpoint.
 */
inline constexpr int whiteKeyIndexInOctave(int inNote)
{
    constexpr int indices[] = {0, 0, 1, 1, 2, 3, 3, 4, 4, 5, 5, 6};

    return indices[inNote % 12];
}

/** @return The length a JUCE keyboard restricted to this range occupies, along its key axis. */
inline float keyboardLength(const DisplayRange& inRange, float inKeyWidthPx)
{
    const float start = static_cast<float>(octaveOf(inRange.lowNote)) * 7.0f * inKeyWidthPx;
    const float end = (static_cast<float>(octaveOf(inRange.highNote)) * 7.0f
                       + static_cast<float>(whiteKeyIndexInOctave(inRange.highNote)) + 1.0f)
                      * inKeyWidthPx;

    return end - start;
}

/**
 * Which notes the piano roll should show.
 *
 * Two rules, in this order. The range covers every transcribed note, so limiting it never hides
 * one -- it only drops the octaves nothing reached. And it is never narrower than the component,
 * so the keyboard fills its column rather than leaving a gap: that is what decides how far past
 * the notes it opens up, and why an empty roll ends up a little wider than C0-B5.
 *
 * Whole octaves throughout, so the C separators the roll draws land on its edges.
 *
 * @param inLowestPitch Lowest transcribed note, ignored when inHasNotes is false.
 * @param inHighestPitch Highest transcribed note, ignored when inHasNotes is false.
 * @param inHasNotes Whether there is a transcription to fit around at all.
 * @param inAvailableLengthPx The keyboard's extent along its key axis -- its height, here.
 * @param inKeyWidthPx One white key's extent along the same axis.
 */
inline DisplayRange computeDisplayRange(
    int inLowestPitch, int inHighestPitch, bool inHasNotes, float inAvailableLengthPx, float inKeyWidthPx)
{
    int low_octave = DEFAULT_LOW_OCTAVE;
    int high_octave = DEFAULT_HIGH_OCTAVE;

    if (inHasNotes) {
        const int lowest = std::clamp(std::min(inLowestPitch, inHighestPitch), MIN_MIDI_NOTE, MAX_MIDI_NOTE);
        const int highest = std::clamp(std::max(inLowestPitch, inHighestPitch), MIN_MIDI_NOTE, MAX_MIDI_NOTE);

        low_octave = octaveOf(lowest);
        high_octave = octaveOf(highest);
    }

    auto range_for = [](int inLowOctave, int inHighOctave) {
        return DisplayRange {inLowOctave * 12, std::min(inHighOctave * 12 + 11, MAX_MIDI_NOTE)};
    };

    // Widen an octave at a time, above first so the notes sit low in the view rather than high,
    // until the keyboard is at least as long as the space it has. Bounded by the MIDI range on
    // both sides, so a roll taller than 128 keys simply shows all of them.
    if (inKeyWidthPx > 0.0f) {
        bool grow_upwards = true;

        while (keyboardLength(range_for(low_octave, high_octave), inKeyWidthPx) < inAvailableLengthPx
               && (low_octave > 0 || high_octave < LAST_OCTAVE)) {
            if (grow_upwards && high_octave < LAST_OCTAVE) {
                high_octave++;
            } else if (low_octave > 0) {
                low_octave--;
            } else {
                high_octave++;
            }

            grow_upwards = !grow_upwards;
        }
    }

    return range_for(low_octave, high_octave);
}

/**
 * The range to show while a transcription is streaming in: the one the notes so far ask for, but
 * never narrower than what is already on screen. Without this the view would jump every time a
 * chunk introduced a note outside the previous span.
 */
inline DisplayRange unionOf(const DisplayRange& inA, const DisplayRange& inB)
{
    return {std::min(inA.lowNote, inB.lowNote), std::max(inA.highNote, inB.highNote)};
}
} // namespace PianoRollRange

#endif // NN_PIANOROLLRANGE_H
