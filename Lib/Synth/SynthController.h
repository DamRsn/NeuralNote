//
// Created by Damien Ronssin on 10.06.23.
//

#ifndef SynthController_h
#define SynthController_h

#include <SynthVoice.h>
#include "Notes.h"

enum EventType
{
    NoteOn,
    NoteOff,
    PitchBend
};

struct SingleEvent
{
    double time = 0.0;
    int midiNote = 0;
    EventType eventType = NoteOff;
    float bend = 0.0f;
};

class SynthController
{
public:
    explicit SynthController(AudioProcessor* inProcessor);

    void setSampleRate(double inSampleRate);

    static std::vector<MidiMessage> buildSingleEventVector(const std::vector<Notes::Event>& inNoteEvents);

    void setNewEventVectorToUse(std::vector<MidiMessage>& inEvents);

    const MidiBuffer& generateNextMidiBuffer(int inNumSamples);

    void reset();

    void setNewPhase(double inNewTime);

private:
    void _sanitizeVoices();

    void _updateCurrentEventIndex();

    bool _isNextOnOffEventNoteOff(int inMidiNote);

    AudioProcessor* mProcessor;

    MidiBuffer mMidiBuffer;

    std::vector<MidiMessage> mEvents;
    size_t mCurrentEventIndex = 0;

    unsigned long long mCurrentSampleIndex = 0;
    double mCurrentTime = 0.0;
    double mSampleRate;
};

#endif // SynthController_h
