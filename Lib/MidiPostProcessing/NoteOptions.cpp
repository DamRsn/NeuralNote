//
// Created by Damien Ronssin on 13.03.23.
//

#include "NoteOptions.h"

void NoteOptions::setParameters(
    RootNote inRootNote, ScaleType inScaleType, SnapMode inSnapMode, int inMinMidiNote, int inMaxMidiNote)
{
    mRootNote = inRootNote;
    mScaleType = inScaleType;
    mSnapMode = inSnapMode;
    mMinMidiNote = inMinMidiNote;
    mMaxMidiNote = inMaxMidiNote;
}

std::vector<Notes::Event> NoteOptions::process(const std::vector<Notes::Event>& inNoteEvents)
{
    std::vector<Notes::Event> processed_note_events;

    // Set key array
    auto key_array = _createKeyArray(mRootNote, mScaleType);

    processed_note_events.reserve(inNoteEvents.size());

    for (const auto& note_event: inNoteEvents) {
        if (note_event.pitch < mMinMidiNote || note_event.pitch > mMaxMidiNote)
            continue;

        if (mScaleType == Chromatic) {
            processed_note_events.push_back(note_event);
        } else {
            if (mSnapMode == Remove) {
                if (_isInKey(note_event.pitch, key_array))
                    processed_note_events.push_back(note_event);
            } else {
                auto processed_note_event = note_event;

                // If pitch bends are more positive: adjust up, otherwise adjust down.
                // Nothing is done if note is in key.
                bool adjust_up = std::accumulate(note_event.bends.begin(), note_event.bends.end(), 0) >= 0;
                processed_note_event.pitch = _getClosestMidiNoteInKey(note_event.pitch, key_array, adjust_up);

                processed_note_events.push_back(processed_note_event);
            }
        }
    }

    return processed_note_events;
}

bool NoteOptions::_isInKey(int inMidiNote, const std::array<int, 7>& inKeyArray)
{
    auto note_index = _midiToNoteIndex(inMidiNote);

    for (int i = 0; i < 7; i++) {
        if (note_index == inKeyArray[i])
            return true;
    }

    return false;
}

int NoteOptions::_getClosestMidiNoteInKey(int inMidiNote, const std::array<int, 7>& inKeyArray, bool inAdjustUp)
{
    if (_isInKey(inMidiNote, inKeyArray))
        return inMidiNote;

    if (inAdjustUp) {
        if (inMidiNote < MAX_MIDI_NOTE - 1)
            return inMidiNote + 1;
        else
            return inMidiNote - 1;
    } else {
        if (inMidiNote > MIN_MIDI_NOTE)
            return inMidiNote - 1;
        else
            return inMidiNote + 1;
    }
}

int NoteOptions::_midiToNoteIndex(int inMidiNote)
{
    return inMidiNote % 12;
}

std::array<int, 7> NoteOptions::_createKeyArrayForScale(int rootNoteIndex, const std::array<int, 7>& intervals)
{
    std::array<int, 7> key_array {};
	for (size_t i = 0; i < sizeof(intervals) / sizeof(int); i++)
		key_array[i] = (rootNoteIndex + intervals[i]) % 12;

    return key_array;
}


std::array<int, 7> NoteOptions::_createKeyArray(RootNote inRootNote, ScaleType inScaleType)
{
    auto root_note_idx = _rootNoteToNoteIdx(inRootNote);

    switch (inScaleType) {
        case Major:
            return _createKeyArrayForScale(root_note_idx, MAJOR_SCALE_INTERVALS);
        case Minor:
            return _createKeyArrayForScale(root_note_idx, MINOR_SCALE_INTERVALS);
        case Dorian:
            return _createKeyArrayForScale(root_note_idx, DORIAN_SCALE_INTERVALS);
        case Mixolydian:
            return _createKeyArrayForScale(root_note_idx, MIXOLYDIAN_SCALE_INTERVALS);
        case Lydian:
            return _createKeyArrayForScale(root_note_idx, LYDIAN_SCALE_INTERVALS);
        case Phrygian:
            return _createKeyArrayForScale(root_note_idx, PHRYGIAN_SCALE_INTERVALS);
        case Locrian:
            return _createKeyArrayForScale(root_note_idx, LOCRIAN_SCALE_INTERVALS);
        case MinorBlues:
            return _createKeyArrayForScale(root_note_idx, MINOR_BLUES_SCALE_INTERVALS);
        case MinorPentatonic:
            return _createKeyArrayForScale(root_note_idx, MINOR_PENTATONIC_SCALE_INTERVALS);
        case MajorPentatonic:
            return _createKeyArrayForScale(root_note_idx, MAJOR_PENTATONIC_SCALE_INTERVALS);
        case MelodicMinor:
            return _createKeyArrayForScale(root_note_idx, MELODIC_MINOR_SCALE_INTERVALS);
        case HarmonicMinor:
            return _createKeyArrayForScale(root_note_idx, HARMONIC_MINOR_SCALE_INTERVALS);
        case HarmonicMajor:
            return _createKeyArrayForScale(root_note_idx, HARMONIC_MAJOR_SCALE_INTERVALS);
        default:
            // If chromatic, array should not be used.
            return {};
    }
}

int NoteOptions::_rootNoteToNoteIdx(RootNote inRootNote)
{
    return (static_cast<int>(inRootNote) + 12 - 3) % 12;
}
