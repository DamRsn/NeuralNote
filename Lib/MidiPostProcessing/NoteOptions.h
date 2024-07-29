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
    void setParameters(bool inEnable,
                       RootNote inRootNote,
                       ScaleType inScaleType,
                       SnapMode inSnapMode,
                       int inMinMidiNote,
                       int inMaxMidiNote);

    std::vector<Notes::Event> process(const std::vector<Notes::Event>& inNoteEvents);

private:
    bool mEnable = false;
    RootNote mRootNote = C;
    ScaleType mScaleType = Chromatic;
    SnapMode mSnapMode = Remove;
    int mMinMidiNote = MIN_MIDI_NOTE;
    int mMaxMidiNote = MAX_MIDI_NOTE;

    template <std::size_t N>
    static std::vector<int> _createKeyVectorForScale(int rootNoteIndex, const std::array<int, N> intervals)
    {
        std::vector<int> keyVector {};
        for (const auto& interval: intervals) {
            keyVector.push_back(rootNoteIndex + interval);
        }

        return keyVector;
    }

    static std::vector<int> _createKeyVector(RootNote inRootNote, ScaleType inScaleType);

    static bool _isInKey(int inMidiNote, const std::vector<int>& inKeyArray);

    static int _getClosestMidiNoteInKey(int inMidiNote, const std::vector<int>& inKeyArray, bool inAdjustUp);

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
    static constexpr std::array<int, 6> MINOR_BLUES_SCALE_INTERVALS = {0, 3, 5, 6, 7, 10};
    static constexpr std::array<int, 5> MINOR_PENTATONIC_SCALE_INTERVALS = {0, 3, 5, 7, 10};
    static constexpr std::array<int, 5> MAJOR_PENTATONIC_SCALE_INTERVALS = {0, 2, 4, 7, 9};
    static constexpr std::array<int, 7> MELODIC_MINOR_SCALE_INTERVALS = {0, 2, 3, 5, 7, 8, 10};
    static constexpr std::array<int, 7> HARMONIC_MINOR_SCALE_INTERVALS = {0, 2, 3, 5, 7, 8, 11};
    static constexpr std::array<int, 7> HARMONIC_MAJOR_SCALE_INTERVALS = {0, 2, 4, 5, 7, 8, 11};
};

#endif // NoteOptions_h
