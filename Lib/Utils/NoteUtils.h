//
// Created by Damien Ronssin on 14.03.23.
//

#ifndef NN_NOTEUTILS_H
#define NN_NOTEUTILS_H

#include <JuceHeader.h>

namespace NoteUtils
{
static const juce::StringArray RootNotesSharpStr {"A", "A#", "B", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#"};

static const juce::StringArray RootNotesFlatStr {"A", "Bb", "B", "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab"};

enum RootNote { A = 0, A_sharp, B, C, C_sharp, D, D_sharp, E, F_sharp, F, G_sharp, G, TotalNumRootNotes };

static const juce::StringArray ScaleTypesStr {"Chromatic",
                                              "Major",
                                              "Minor",
                                              "Dorian",
                                              "Mixolydian",
                                              "Lydian",
                                              "Phrygian",
                                              "Locrian",
                                              "Minor Blues",
                                              "Minor Pentatonic",
                                              "Major Pentatonic",
                                              "Melodic Minor",
                                              "Harmonic Minor",
                                              "Harmonic Major"};

enum ScaleType {
    Chromatic = 0,
    Major,
    Minor,
    Dorian,
    Mixolydian,
    Lydian,
    Phrygian,
    Locrian,
    MinorBlues,
    MinorPentatonic,
    MajorPentatonic,
    MelodicMinor,
    HarmonicMinor,
    HarmonicMajor,
    TotalNumScaleTypes
};

static const juce::StringArray SnapModesStr {"Adjust", "Remove"};

enum SnapMode { Adjust = 0, Remove };

static String midiNoteToStr(int inNoteNumber)
{
    const int octave = (inNoteNumber / 12) - 1;
    const int noteIndex = (inNoteNumber + 3) % 12;
    auto noteName = RootNotesSharpStr[noteIndex];
    noteName += String(octave);
    return noteName;
}

/**
 * Return closest midi note number to frequency
 * @param hz Input frequency
 * @return Closest midi note number
 */
static inline int hzToMidi(float hz)
{
    return (int) std::round(12.0f * std::log2(hz / 440.0f) + 69.0f);
}

/**
 * Compute frequency in Hz corresponding to given midi note
 * @param inMidiNote Midi note number
 * @return Frequency in Hz
 */
static inline float midiToHz(float inMidiNote)
{
    return 440.0f * std::pow(2.0f, (inMidiNote - 69.0f) / 12.0f);
}

} // namespace NoteUtils

#endif //NN_NOTEUTILS_H
