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
    MidiFileWriter();

    bool writeMidiFile(const std::vector<Notes::Event>& inNoteEvents, int inTempoBPM);

private:
    static double _BPMToMicrosecondsPerQuarterNote(double inTempoBPM);

    juce::File mOutFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                              .getChildFile("NeuralNote");


};

#endif // MidiFileWriter_h
