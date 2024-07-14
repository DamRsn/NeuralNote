#pragma once

#include "atomic"
#include <JuceHeader.h>

#include "Resampler.h"
#include "ProcessorBase.h"
#include "BasicPitch.h"
#include "NoteOptions.h"
#include "MidiFileWriter.h"
#include "RhythmOptions.h"
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

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;

    void getStateInformation(juce::MemoryBlock& destData) override;

    void setStateInformation(const void* data, int sizeInBytes) override;

    State getState() const { return mState.load(); }

    void setStateToRecording() { mState.store(Recording); }

    void setStateToProcessing() { mState.store(Processing); }

    void setStateToPopulatedAudioAndMidiRegions() { mState.store(PopulatedAudioAndMidiRegions); }

    void clear();

    const juce::Optional<juce::AudioPlayHead::PositionInfo>&
        getPlayheadInfoOnRecordStart(); // TODO: Add to timeQuantizeManager

    // TODO: TimeQuantizeManager
    bool canQuantize() const;

    std::string getTempoStr() const;

    std::string getTimeSignatureStr() const;

    SourceAudioManager* getSourceAudioManager();

    Player* getPlayer();

    TranscriptionManager* getTranscriptionManager();

    std::array<RangedAudioParameter*, ParameterHelpers::TotalNumParams>& getParams();

    float getParameterValue(ParameterHelpers::ParamIdEnum inParamId) const;

    AudioProcessorValueTreeState& getAPVTS() { return mAPVTS; }

    NeuralNoteMainView* getNeuralNoteMainView() const;

    double getCurrentTempo() const { return mCurrentTempo.load(); }

private:
    // Value tree state to pass automatable parameters from UI
    juce::AudioProcessorValueTreeState mAPVTS;

    std::array<RangedAudioParameter*, ParameterHelpers::TotalNumParams> mParams {};

    std::atomic<State> mState = EmptyAudioAndMidiRegions;

    std::unique_ptr<SourceAudioManager> mSourceAudioManager;
    std::unique_ptr<Player> mPlayer;
    std::unique_ptr<TranscriptionManager> mTranscriptionManager;

    bool mWasRecording = false;

    std::atomic<double> mCurrentTempo = -1.0;
    std::atomic<int> mCurrentTimeSignatureNum = -1;
    std::atomic<int> mCurrentTimeSignatureDenom = -1;

    // To quantize manager
    juce::Optional<juce::AudioPlayHead::PositionInfo> mPlayheadInfoStartRecord;
};
