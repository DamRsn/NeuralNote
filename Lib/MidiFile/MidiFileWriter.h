//
// Created by Damien Ronssin on 11.03.23.
//

#ifndef MidiFileWriter_h
#define MidiFileWriter_h

#include <JuceHeader.h>

#include "Notes.h"

class MidiFileWriter
{
public:
    bool writeMidiFile(const std::vector<Notes::Event>& inNoteEvents,
                       juce::File& fileToUse,
                       int inTempoBPM);

private:
    static double _BPMToMicrosecondsPerQuarterNote(double inTempoBPM);
};

#endif // MidiFileWriter_h
