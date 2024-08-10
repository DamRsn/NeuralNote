#pragma once

#include <atomic>
#include <JuceHeader.h>

#include "Resampler.h"
#include "ProcessorBase.h"
#include "BasicPitch.h"
#include "NoteOptions.h"
#include "MidiFileWriter.h"
#include "TimeQuantizeOptions.h"
#include "Player.h"
#include "SourceAudioManager.h"
#include "ParameterHelpers.h"
#include "TranscriptionManager.h"
#include "NnId.h"

class NeuralNoteMainView;
class NeuralNoteEditor;

enum State { EmptyAudioAndMidiRegions = 0, Recording, Processing, PopulatedAudioAndMidiRegions };

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

    void setStateToRecording() { mState.store(Recording); }

    void setStateToProcessing() { mState.store(Processing); }

    void setStateToPopulatedAudioAndMidiRegions() { mState.store(PopulatedAudioAndMidiRegions); }

    void clear();

    SourceAudioManager* getSourceAudioManager() const;

    Player* getPlayer() const;

    TranscriptionManager* getTranscriptionManager() const;

    std::array<RangedAudioParameter*, ParameterHelpers::TotalNumParams>& getParams();

    float getParameterValue(ParameterHelpers::ParamIdEnum inParamId) const;

    NeuralNoteMainView* getNeuralNoteMainView() const;

    AudioProcessorValueTreeState& getAPVTS();

    ValueTree& getValueTree();

    void addListenerToStateValueTree(ValueTree::Listener* inListener);

    void removeListenerFromStateValueTree(ValueTree::Listener* inListener);

private:
    static ValueTree _createDefaultValueTree();

    void _updateValueTree(const ValueTree& inNewState);

    // ValueTree for general plugin state
    ValueTree mValueTree = _createDefaultValueTree();

    // Value tree state to pass automatable parameters from UI
    AudioProcessorValueTreeState mAPVTS;

    std::array<RangedAudioParameter*, ParameterHelpers::TotalNumParams> mParams {};

    std::atomic<State> mState = EmptyAudioAndMidiRegions;

    std::unique_ptr<SourceAudioManager> mSourceAudioManager;
    std::unique_ptr<Player> mPlayer;
    std::unique_ptr<TranscriptionManager> mTranscriptionManager;
    std::unique_ptr<FileLogger> mLogger;
};
