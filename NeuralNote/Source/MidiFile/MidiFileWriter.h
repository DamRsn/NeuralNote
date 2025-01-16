//
// Created by Damien Ronssin on 11.03.23.
//

#ifndef MidiFileWriter_h
#define MidiFileWriter_h

#include <JuceHeader.h>

#include "BasicPitchConstants.h"
#include "Notes.h"
#include "TimeQuantizeOptions.h"

class MidiFileWriter
{
public:
    bool writeMidiFile(const std::vector<Notes::Event>& inNoteEvents,
                       const File& fileToUse,
                       const TimeQuantizeOptions::TimeQuantizeInfo& inInfo,
                       double inExportBpm,
                       PitchBendModes inPitchBendMode) const;

private:
    static double _BPMToMicrosecondsPerQuarterNote(double inTempoBPM);

    const int mTicksPerQuarterNote = 960;
};

#endif // MidiFileWriter_h
