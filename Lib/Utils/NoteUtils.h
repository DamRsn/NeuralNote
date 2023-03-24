//
// Created by Damien Ronssin on 14.03.23.
//

#ifndef NN_NOTEUTILS_H
#define NN_NOTEUTILS_H

#include <JuceHeader.h>

namespace NoteUtils
{
static const juce::StringArray RootNotesSharpStr {
    "A", "A#", "B", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#"};

static const juce::StringArray RootNotesFlatStr {
    "A", "Bb", "B", "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab"};

enum RootNote
{
    A = 0,
    A_sharp,
    B,
    C,
    C_sharp,
    D,
    D_sharp,
    E,
    F_sharp,
    F,
    G_sharp,
    G,
    TotalNumRootNotes
};

static const juce::StringArray ScaleTypesStr {"Chromatic", "Major", "Minor"};

enum ScaleType
{
    Chromatic = 0,
    Major,
    Minor,
    TotalNumScaleTypes
};

static const juce::StringArray SnapModesStr {"Adjust", "Remove"};

enum SnapMode
{
    Adjust = 0,
    Remove
};

static String midiNoteToStr(int inNoteNumber)
{
    const int octave = (inNoteNumber / 12) - 1;
    const int noteIndex = (inNoteNumber + 3) % 12;
    auto noteName = RootNotesSharpStr[noteIndex];
    noteName += String(octave);
    return noteName;
}

} // namespace NoteUtils

#endif //NN_NOTEUTILS_H
