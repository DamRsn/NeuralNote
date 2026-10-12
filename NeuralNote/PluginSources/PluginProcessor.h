#pragma once

#include <atomic>
#include <JuceHeader.h>

#include "Resampler.h"
#include "ProcessorBase.h"
#include "ModelDownloader.h"
#include "MuscriptorEngine.h"
#include "InstrumentMixer.h"
#include "Player.h"
#include "SourceAudioManager.h"
#include "ParameterHelpers.h"
#include "TranscriptionManager.h"
#include "NnId.h"
#include "NnLook.h"

class NeuralNoteMainView;
class NeuralNoteEditor;

/**
 * AudioLoaded is audio in, nothing transcribed: the transport works, the piano roll is empty, and
 * the model has not been asked to run. Loading a file lands here rather than starting a
 * transcription, because a run takes minutes and the user gets to pick instruments first. A run
 * that is discarded, or stops before its first chunk, returns here with the audio kept.
 *
 * Paused is a transcription stopped part way, with no job running: its notes are playable up to
 * TranscriptionManager::getFinalizedThrough, and it can be resumed from there. A run that is paused
 * or fails after its first chunk lands here, and so does reopening a state saved while one was running.
 */
enum State { EmptyAudioAndMidiRegions = 0, Recording, AudioLoaded, Processing, Paused, PopulatedAudioAndMidiRegions };

class NeuralNoteAudioProcessor : public PluginHelpers::ProcessorBase
{
public:
    NeuralNoteAudioProcessor();

    ~NeuralNoteAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;

    void processBlock(AudioBuffer<float>&, MidiBuffer&) override;

    AudioProcessorEditor* createEditor() override;

    void getStateInformation(MemoryBlock& destData) override;

    void setStateInformation(const void* data, int sizeInBytes) override;

    State getState() const { return mState.load(); }

    /**
     * @return Whether there is audio to play, and a piano roll and a time axis to draw against it.
     *         True from the moment a file is loaded: the transcription is what fills the roll, not
     *         what makes the audio audible. In Processing it is a partial result -- a transcription
     *         publishes its notes chunk by chunk, and TranscriptionManager::getFinalizedThrough
     *         says how much of it is there; past that line the roll is empty and the synth silent.
     *         Paused is the same, with no job running.
     */
    bool canPlay() const
    {
        const State state = mState.load();
        return state == AudioLoaded || state == Processing || state == Paused || state == PopulatedAudioAndMidiRegions;
    }

    /** @return Whether there are notes to show and hear -- partial ones included. */
    bool hasTranscription() const
    {
        const State state = mState.load();
        return state == Processing || state == Paused || state == PopulatedAudioAndMidiRegions;
    }

    /** @return Whether a transcription has been started and not finished: running or paused. */
    bool isTranscriptionUnfinished() const
    {
        const State state = mState.load();
        return state == Processing || state == Paused;
    }

    void setStateToRecording() { mState.store(Recording); }

    void setStateToAudioLoaded() { mState.store(AudioLoaded); }

    void setStateToProcessing() { mState.store(Processing); }

    void setStateToPaused() { mState.store(Paused); }

    void setStateToPopulatedAudioAndMidiRegions() { mState.store(PopulatedAudioAndMidiRegions); }

    /** Drops the audio and the transcription both, back to an empty window. */
    void clear();

    /**
     * Drops the transcription and keeps the audio, landing in AudioLoaded so it can be run again.
     * What a cancelled or failed run does, and what the bin's "transcription only" entry does.
     */
    void clearTranscription();

    SourceAudioManager* getSourceAudioManager() const;

    Player* getPlayer() const;

    TranscriptionManager* getTranscriptionManager() const;

    /** The same object in every instance of this process. */
    ModelDownloader& getModelDownloader() { return *mModelDownloader; }

    InstrumentMixer* getInstrumentMixer() const;

    float getParameterValue(ParameterHelpers::ParamIdEnum inParamId) const;

    NeuralNoteMainView* getNeuralNoteMainView() const;

    AudioProcessorValueTreeState& getAPVTS();

    ValueTree& getValueTree();

    void addListenerToStateValueTree(ValueTree::Listener* inListener);

    void removeListenerFromStateValueTree(ValueTree::Listener* inListener);

private:
    static ValueTree _createDefaultValueTree();

    void _updateValueTree(const ValueTree& inNewState);

    // First, so it is the last thing destroyed: it is still the default look and feel while the
    // rest of this is going away. Shared because there is one default per binary rather than one
    // per instance, so two editors open at once have to be looking at the same object.
    SharedResourcePointer<nn::DefaultLookAndFeel> mDefaultLookAndFeel;

    // Shared for the same reason: two instances show one download rather than each writing its own
    // to the same file.
    SharedResourcePointer<ModelDownloader> mModelDownloader;

    // ValueTree for general plugin state
    ValueTree mValueTree = _createDefaultValueTree();

    // Value tree state to pass automatable parameters from UI
    AudioProcessorValueTreeState mAPVTS;

    std::array<RangedAudioParameter*, ParameterHelpers::TotalNumParams> mParams {};

    std::atomic<State> mState = EmptyAudioAndMidiRegions;

    std::unique_ptr<SourceAudioManager> mSourceAudioManager;
    std::unique_ptr<Player> mPlayer;
    std::unique_ptr<TranscriptionManager> mTranscriptionManager;
    std::unique_ptr<InstrumentMixer> mInstrumentMixer;
    std::unique_ptr<FileLogger> mLogger;
};
