//
// Created by Damien Ronssin on 11.03.23.
//

#ifndef MidiFileWriter_h
#define MidiFileWriter_h

#include <JuceHeader.h>

#include "Notes.h"

enum AudioSampleAcquisitionMode
{
    NoSampleAcquired = 0,
    RecordedPlaying,
    RecordedNotPlaying,
    FileDrop
};

class MidiFileWriter
{
public:
    bool writeMidiFile(const std::vector<Notes::Event>& inNoteEvents,
                       juce::File& fileToUse,
                       const juce::AudioPlayHead::CurrentPositionInfo& inInfoStart,
                       AudioSampleAcquisitionMode inSampleAcquisitionMode);

private:
    static double _BPMToMicrosecondsPerQuarterNote(double inTempoBPM);

    const int mTicksPerQuarterNote = 960;
};

#endif // MidiFileWriter_h
