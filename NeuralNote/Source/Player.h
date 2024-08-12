//
// Created by Damien Ronssin on 11.06.23.
//

#ifndef Player_h
#define Player_h

#include <JuceHeader.h>

#include "SynthController.h"
#include "SynthVoice.h"

class NeuralNoteAudioProcessor;

class Player : public ValueTree::Listener
{
public:
    explicit Player(NeuralNoteAudioProcessor* inProcessor);

    ~Player() override;

    void prepareToPlay(double inSampleRate, int inSamplesPerBlock);

    void processBlock(AudioBuffer<float>& inAudioBuffer, MidiBuffer& outMidiBuffer);

    bool isPlaying() const;

    void setPlayingState(bool inIsPlaying);

    void reset();

    /**
     * Sets the new playhead position (in seconds).
     * Nothing is performed if inNewPosition is out of bounds (less than 0 or larger than audio length available)
     * @param inNewPosition New playhead position in seconds
     */
    void setPlayheadPositionSeconds(double inNewPosition);

    double getPlayheadPositionSeconds() const;

    SynthController* getSynthController() const;

    void saveStateToValueTree();

    static constexpr int NUM_VOICES_SYNTH = 16;

private:
    void valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property) override;

    void _setGains(float inGainAudioSourceDB, float inGainSynthDB);

    void _clearActiveNotesMidiOut(MidiBuffer& outMidiBuffer);

    std::atomic<bool> mIsPlaying = false;
    bool mWasPlaying = false;
    NeuralNoteAudioProcessor* mProcessor;

    std::unique_ptr<SynthController> mSynthController;
    std::unique_ptr<MPESynthesiser> mSynth;

    AudioBuffer<float> mInternalBuffer;

    double mPlayheadTime = 0;
    double mSampleRate = 44100;

    float mGainSourceAudio = 0;
    float mGainSynth = 0;

    bool mShouldOutputMidi = false;
    bool mWasOutputtingMidi = false;

    std::array<int, 128> mActiveNotesMidiOut {};
};

#endif // Player_h
