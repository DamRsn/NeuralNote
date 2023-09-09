//
// Created by Damien Ronssin on 13.03.23.
//

#ifndef NoteOptions_h
#define NoteOptions_h

#include <JuceHeader.h>
#include "Notes.h"
#include "NoteUtils.h"

using namespace NoteUtils;

class NoteOptions
{
public:
    void setParameters(
        RootNote inRootNote, ScaleType inScaleType, SnapMode inSnapMode, int inMinMidiNote, int inMaxMidiNote);

    std::vector<Notes::Event> process(const std::vector<Notes::Event>& inNoteEvents);

private:
    RootNote mRootNote = C;
    ScaleType mScaleType = Chromatic;
    SnapMode mSnapMode = Remove;
    int mMinMidiNote = MIN_MIDI_NOTE;
    int mMaxMidiNote = MAX_MIDI_NOTE;

	static std::array<int, 7> _createKeyArrayForScale(int rootNoteIndex, const std::array<int, 7>& intervals);
    static std::array<int, 7> _createKeyArray(RootNote inRootNote, ScaleType inScaleType);

    static bool _isInKey(int inMidiNote, const std::array<int, 7>& inKeyArray);

    static int _getClosestMidiNoteInKey(int inMidiNote, const std::array<int, 7>& inKeyArray, bool inAdjustUp);

    static int _rootNoteToNoteIdx(RootNote inRootNote);

    /* Returns the note index (between 0 and 11 included, C to B) given midi note number */
    static int _midiToNoteIndex(int inMidiNote);

    static constexpr std::array<int, 7> MAJOR_SCALE_INTERVALS = {0, 2, 4, 5, 7, 9, 11};
    static constexpr std::array<int, 7> MINOR_SCALE_INTERVALS = {0, 2, 3, 5, 7, 8, 10};
    static constexpr std::array<int, 7> DORIAN_SCALE_INTERVALS = {0, 2, 3, 5, 7, 9, 10};
    static constexpr std::array<int, 7> MIXOLYDIAN_SCALE_INTERVALS = {0, 2, 4, 5, 7, 9, 10};
    static constexpr std::array<int, 7> LYDIAN_SCALE_INTERVALS = {0, 2, 4, 6, 7, 9, 11};
    static constexpr std::array<int, 7> PHRYGIAN_SCALE_INTERVALS = {0, 1, 3, 5, 7, 8, 10};
    static constexpr std::array<int, 7> LOCRIAN_SCALE_INTERVALS = {0, 1, 3, 5, 6, 8, 10};
    // Add 0's to get all scales to size 7
    static constexpr std::array<int, 7> MINOR_BLUES_SCALE_INTERVALS = {0, 3, 5, 6, 7, 10, 0};
    static constexpr std::array<int, 7> MINOR_PENTATONIC_SCALE_INTERVALS = {0, 3, 5, 7, 10, 0, 0};
    static constexpr std::array<int, 7> MAJOR_PENTATONIC_SCALE_INTERVALS = {0, 2, 4, 7, 9, 0, 0};
    static constexpr std::array<int, 7> MELODIC_MINOR_SCALE_INTERVALS = {0, 2, 3, 5, 7, 8, 10};
    static constexpr std::array<int, 7> HARMONIC_MINOR_SCALE_INTERVALS = {0, 2, 3, 5, 7, 8, 11};
    static constexpr std::array<int, 7> HARMONIC_MAJOR_SCALE_INTERVALS = {0, 2, 4, 5, 7, 8, 11};
};

#endif // NoteOptions_h
