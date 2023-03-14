//
// Created by Damien Ronssin on 13.03.23.
//

#include "NoteOptions.h"

void NoteOptions::setParameters(NoteOptions::RootNote inRootNote,
                                NoteOptions::ScaleType inScaleType,
                                NoteOptions::SnapMode inSnapMode,
                                int inMinMidiNote,
                                int inMaxMidiNote)
{
    mRootNote = inRootNote;
    mScaleType = inScaleType;
    mSnapMode = inSnapMode;
    mMinMidiNote = inMinMidiNote;
    mMaxMidiNote = inMaxMidiNote;
}

std::vector<Notes::Event>
    NoteOptions::processKey(const std::vector<Notes::Event>& inNoteEvents)
{
    std::vector<Notes::Event> processed_note_events;

    if (mScaleType == Chromatic)
    {
        processed_note_events = inNoteEvents;
        return processed_note_events;
    }

    // Set key array
    auto key_array = _createKeyArray(mRootNote, mScaleType);

    processed_note_events.reserve(inNoteEvents.size());

    for (const auto& note_event: inNoteEvents)
    {
        if (note_event.pitch < mMinMidiNote || note_event.pitch > mMaxMidiNote)
            continue;

        if (mSnapMode == Remove)
        {
            if (_isInKey(note_event.pitch, key_array))
                processed_note_events.push_back(note_event);
        }
        else
        {
            auto processed_note_event = note_event;

            // TODO: determine adjust up based on pitch bends.
            processed_note_event.pitch =
                _getClosestMidiNoteInKey(note_event.pitch, key_array, true);

            processed_note_events.push_back(processed_note_event);
        }
    }

    return processed_note_events;
}

bool NoteOptions::_isInKey(int inMidiNote, const std::array<int, 7>& inKeyArray)
{
    auto note_index = _midiToNoteIndex(inMidiNote);

    for (int i = 0; i < 7; i++)
    {
        if (note_index == inKeyArray[i])
            return true;
    }

    return false;
}

int NoteOptions::_getClosestMidiNoteInKey(int inMidiNote,
                                          const std::array<int, 7>& inKeyArray,
                                          bool inAdjustUp)
{
    if (_isInKey(inMidiNote, inKeyArray))
        return inMidiNote;

    if (inAdjustUp)
    {
        if (inMidiNote < MAX_MIDI_NOTE - 1)
            return inMidiNote + 1;
        else
            return inMidiNote - 1;
    }
    else
    {
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

std::array<int, 7> NoteOptions::_createKeyArray(NoteOptions::RootNote inRootNote,
                                                NoteOptions::ScaleType inScaleType)
{
    auto root_note_idx = _rootNoteToNoteIdx(inRootNote);
    std::array<int, 7> key_array {};

    if (inScaleType == Major)
    {
        for (size_t i = 0; i < 7; i++)
            key_array[i] = (root_note_idx + MAJOR_SCALE_INTERVALS[i]) % 12;
    }
    else if (inScaleType == Minor)
    {
        for (size_t i = 0; i < 7; i++)
            key_array[i] = (root_note_idx + MINOR_SCALE_INTERVALS[i]) % 12;
    }
    else
    {
        jassertfalse;
    }

    return key_array;
}

int NoteOptions::_rootNoteToNoteIdx(NoteOptions::RootNote inRootNote)
{
    return (static_cast<int>(inRootNote) + 12 - 3) % 12;
}
