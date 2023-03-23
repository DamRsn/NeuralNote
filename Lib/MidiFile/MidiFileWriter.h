//
// Created by Damien Ronssin on 11.03.23.
//

#ifndef MidiFileWriter_h
#define MidiFileWriter_h

#include <JuceHeader.h>

#include "Constants.h"
#include "Notes.h"

class MidiFileWriter
{
public:
    bool writeMidiFile(
        const std::vector<Notes::Event>& inNoteEvents,
        juce::File& fileToUse,
        const juce::Optional<juce::AudioPlayHead::PositionInfo>& inInfoStart,
        double inBPM,
        PitchBendModes inPitchBendMode) const;

private:
    static double _BPMToMicrosecondsPerQuarterNote(double inTempoBPM);

    const int mTicksPerQuarterNote = 960;
};

#endif // MidiFileWriter_h
