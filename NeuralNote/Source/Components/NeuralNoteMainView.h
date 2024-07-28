//
// Created by Damien Ronssin on 06.03.23.
//

#ifndef PluginMainView_h
#define PluginMainView_h

#include <JuceHeader.h>

#include "PluginProcessor.h"
#include "Knob.h"
#include "NoteOptionsView.h"
#include "RhythmOptionsView.h"
#include "TranscriptionOptionsView.h"
#include "VisualizationPanel.h"
#include "NeuralNoteLNF.h"
#include "NnId.h"

class NeuralNoteMainView
    : public juce::Component
    , public juce::Timer
    , public juce::ValueTree::Listener
{
public:
    explicit NeuralNoteMainView(NeuralNoteAudioProcessor& processor);

    ~NeuralNoteMainView();

    void resized() override;

    void paint(juce::Graphics& g) override;

    void timerCallback() override;

    void repaintPianoRoll();

private:
    void updateEnablements();

    void valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property) override;

    NeuralNoteAudioProcessor& mProcessor;
    NeuralNoteLNF mLNF;

    State mPrevState = EmptyAudioAndMidiRegions;

    VisualizationPanel mVisualizationPanel;
    TranscriptionOptionsView mTranscriptionOptions;
    NoteOptionsView mNoteOptions;
    RhythmOptionsView mQuantizePanel;

    std::unique_ptr<juce::TextButton> mMuteButton;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> mMuteButtonAttachment;

    std::unique_ptr<juce::DrawableButton> mRecordButton;
    std::unique_ptr<juce::DrawableButton> mClearButton;

    std::unique_ptr<juce::DrawableButton> mBackButton;
    std::unique_ptr<juce::DrawableButton> mPlayPauseButton;
    std::unique_ptr<juce::DrawableButton> mCenterButton;
    std::unique_ptr<juce::DrawableButton> mSettingsButton;

    std::unique_ptr<Knob> mMinNoteSlider;
    std::unique_ptr<Knob> mMaxNoteSlider;

    std::unique_ptr<ComboBox> mKey; // C, C#, D, D# ...
    std::unique_ptr<ComboBox> mMode; // Major, Minor, Chromatic

    int mNumCallbacksStuckInProcessingState = 0;
};

#endif // PluginMainView_h
