//
// Created by Damien Ronssin on 10.06.23.
//

#ifndef SynthController_h
#define SynthController_h

#include <JuceHeader.h>

#include "SynthVoice.h"
#include "Notes.h"

class NeuralNoteAudioProcessor;

class SynthController
{
public:
    SynthController(NeuralNoteAudioProcessor* inProcessor, MPESynthesiser* inMPESynth);

    void setSampleRate(double inSampleRate);

    static std::vector<MidiMessage> buildMidiEventsVector(const std::vector<Notes::Event>& inNoteEvents);

    void setNewMidiEventsVectorToUse(std::vector<MidiMessage>& inEvents);

    const MidiBuffer& generateNextMidiBuffer(int inNumSamples);

    void reset();

    void setNewTimeSeconds(double inNewTime);

    double getCurrentTimeSeconds() const;

private:
    void _sanitizeVoices();

    void _updateCurrentEventIndex();

    bool _isNextOnOffEventNoteOff(int inMidiNote);

    NeuralNoteAudioProcessor* mProcessor;
    MPESynthesiser* mSynth;

    MidiBuffer mMidiBuffer;

    std::vector<MidiMessage> mEvents;
    size_t mCurrentEventIndex = 0;

    unsigned long long mCurrentSampleIndex = 0;
    std::atomic<double> mCurrentTime = 0.0;
    double mSampleRate = 44100;
};

#endif // SynthController_h
