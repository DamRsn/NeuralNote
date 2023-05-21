//
// Created by Damien Ronssin on 21.05.23.
//

#ifndef Synth_h
#define Synth_h

#include <JuceHeader.h>

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

class Synth
{
public:
    Synth(AudioProcessor* inProcessor);

    ~Synth() = default;

    void prepareToPlay(float inSampleRate, int inSamplesPerBlock);

    void processBlock(juce::AudioBuffer<float>& inAudioBuffer);

    float processSample();

    void reset();

    void noteOn(int inMidiNote, float inInitBend, float inAmplitude);

    void noteOff(int inMidiNote);

    void shutAllVoices();

    static std::vector<SingleEvent>
        buildEventVector(const std::vector<Notes::Event>& inNoteEvents);

    void setNewEventVectorToUse(std::vector<SingleEvent>& inEvents);

private:
    void _sanitizeVoices();

    void _updateCurrentEventIndex();

    bool _isNextOnOffEventNoteOff(int inMidiNote);

    AudioProcessor* mProcessor;

    static constexpr size_t NUM_VOICES = 32;
    std::array<SynthVoice, NUM_VOICES> mVoices;

    std::atomic<bool> isPlaying = false;
    float mSampleRate;

    //    unsigned long long mSampleIndex = 0;

    std::vector<SingleEvent> mEvents;
    size_t mCurrentEventIndex = 0;
    double mCurrentTime = 0.0;
};

#endif // Synth_h
