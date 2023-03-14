//
// Created by Damien Ronssin on 14.03.23.
//

#ifndef AUDIO2MIDIPLUGIN_NOTEUTILS_H
#define AUDIO2MIDIPLUGIN_NOTEUTILS_H

#include <JuceHeader.h>

namespace NoteUtils
{
static const juce::StringArray RootNotesSharpStr {
    "A", "A#", "B", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#"};

static const juce::StringArray RootNotesFlatStr {
    "A", "Bb", "B", "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab"};

static const juce::StringArray ScaleTypesStr {"Chromatic", "Major", "Minor"};

static const juce::StringArray SnapModesStr {"Adjust", "Remove"};

static String midiNoteToStr(int inNoteNumber)
{
    const int octave = (inNoteNumber / 12) - 1;
    const int noteIndex = (inNoteNumber + 3) % 12;
    auto noteName = RootNotesSharpStr[noteIndex];
    noteName += String(octave);
    return noteName;
}

} // namespace NoteUtils

#endif //AUDIO2MIDIPLUGIN_NOTEUTILS_H
