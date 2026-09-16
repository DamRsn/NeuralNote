//
// Created by Damien Ronssin on 11.06.23.
//

#ifndef Player_h
#define Player_h

#include <atomic>
#include <cstdint>

#include <JuceHeader.h>

#include "InstrumentSynth.h"
#include "RmsMeter.h"
#include "SynthController.h"

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

    /** Stops the transport and rewinds to the start, keeping the transcription. */
    void returnToStart();

    /** Drops the transcription's playback state as well as rewinding. For the clear paths. */
    void reset();

    /**
     * Sets the new playhead position (in seconds).
     * Nothing is performed if inNewPosition is out of bounds (less than 0 or larger than audio length available)
     * @param inNewPosition New playhead position in seconds
     */
    void setPlayheadPositionSeconds(double inNewPosition);

    double getPlayheadPositionSeconds() const;

    SynthController* getSynthController() const;

    InstrumentSynth* getInstrumentSynth() const;

    /**
     * @return The plugin's own output over the last window, as a mean square: the synth plus the
     *         source audio playback, which is what the master meter shows. Message thread.
     */
    float getMasterMeterMeanSquare() const { return mMasterMeanSquare.load(std::memory_order_relaxed); }

    /**
     * @return A counter bumped once per block. Tells "silent" apart from "not being called" -- a
     *         bypassed or suspended plugin never reaches processBlock, and the levels it published
     *         last would otherwise stay on screen.
     */
    std::uint32_t getMeterFrameCount() const { return mMeterFrame.load(std::memory_order_relaxed); }

    void saveStateToValueTree();

private:
    void valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property) override;

    void _setGains(float inMix, float inMasterGainDb);

    std::atomic<bool> mIsPlaying = false;
    NeuralNoteAudioProcessor* mProcessor;

    std::unique_ptr<SynthController> mSynthController;
    std::unique_ptr<InstrumentSynth> mSynth;

    AudioBuffer<float> mInternalBuffer;

    RmsMeter mMasterMeter;
    std::atomic<float> mMasterMeanSquare {0.0f};
    std::atomic<std::uint32_t> mMeterFrame {0};

    double mPlayheadTime = 0;
    double mSampleRate = 44100;

    float mGainSourceAudio = 0;
    float mGainSynth = 0;
    float mMasterGain = 1;

    bool mShouldOutputMidi = false;
    bool mWasOutputtingMidi = false;

    // A stop or a seek asking the synth to go quiet, consumed by the next block. Needed on top of
    // the scheduler's note-offs because the synth ignores those for drums, which are one-shot.
    std::atomic<bool> mShouldSilenceSynth = false;
};

#endif // Player_h
