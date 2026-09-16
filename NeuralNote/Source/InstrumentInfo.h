//
// Created by Damien Ronssin on 07.08.26.
//

#ifndef InstrumentInfo_h
#define InstrumentInfo_h

#include <JuceHeader.h>

#include "muscriptor/note.hpp"

/**
 * How an instrument is named in the UI.
 *
 * The library's own names are snake_case identifiers meant for logs and the tokenizer
 * (`acoustic_piano`, `clean_electric_guitar`), and the sidebar needs a title and a three-letter
 * chip. Both are a presentation choice, so they live here rather than being asked of msl.
 */
struct InstrumentDisplay {
    juce::String name;
    juce::String abbreviation;
};

/**
 * @param inProgram A MIDI program, or msl::DRUM_PROGRAM.
 * @return Its display name and chip abbreviation. Programs outside the model's named groups --
 *         which it can decode but never picks -- fall back to the library's own `program_<n>`.
 */
InstrumentDisplay instrumentDisplayFor(int inProgram);

/** The same by group, for the instrument picker, which offers groups rather than programs. */
InstrumentDisplay instrumentDisplayForGroup(msl::InstrumentGroup inGroup);

/**
 * The instrument's colour, fixed per instrument rather than per position in the sidebar, so it
 * survives a new instrument appearing mid-transcription. Callers inside the plugin should go
 * through InstrumentMixer::colourForProgram, which is the single lookup the chip, the fader and
 * the piano roll share.
 */
juce::Colour instrumentColourFor(int inProgram);

juce::Colour instrumentColourForGroup(msl::InstrumentGroup inGroup);

#endif // InstrumentInfo_h
