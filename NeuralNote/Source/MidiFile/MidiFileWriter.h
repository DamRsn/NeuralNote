//
// Created by Damien Ronssin on 11.03.23.
//

#ifndef MidiFileWriter_h
#define MidiFileWriter_h

#include <map>
#include <vector>

#include <JuceHeader.h>

#include "NoteEvent.h"

/** What to do when a transcription has more melodic instruments than there are channels. */
enum class MidiOverflowMode {
    // Hand the last channels out again. Two tracks on one channel means the later Program Change
    // wins for anything playing the file by channel; a DAW splitting the import by track is
    // unaffected.
    ReuseChannels = 0,
    // Write only the instruments carrying the most notes.
    DropExtraInstruments = 1,
};

struct MidiChannelMap {
    std::map<int, int> channelForProgram; // program -> MIDI channel, 1-16
    std::vector<int> droppedPrograms; // only ever non-empty in DropExtraInstruments
};

/**
 * Assigns a channel to every instrument in inNoteEvents, in ascending program order so the file
 * matches the sidebar (InstrumentMixer orders the same way).
 *
 * Channel 10 is GM percussion -- a melodic instrument there plays as drums whatever its Program
 * Change says -- so it is reserved for drums even when the transcription has none. That leaves 15
 * melodic channels; past those, inMode decides.
 */
MidiChannelMap buildChannelMap(const std::vector<NoteEvent>& inNoteEvents, MidiOverflowMode inMode);

class MidiFileWriter
{
public:
    bool writeMidiFile(const std::vector<NoteEvent>& inNoteEvents,
                       const File& fileToUse,
                       double inExportBpm,
                       double inStartOffsetSeconds,
                       MidiOverflowMode inOverflowMode) const;

private:
    static double _BPMToMicrosecondsPerQuarterNote(double inTempoBPM);

    const int mTicksPerQuarterNote = 960;
};

#endif // MidiFileWriter_h
