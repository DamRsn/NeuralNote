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

class Audio2MidiMainView
    : public juce::Component
    , public juce::Timer
{
public:
    explicit Audio2MidiMainView(Audio2MidiAudioProcessor& processor);

    void resized() override;

    void paint(juce::Graphics& g) override;

    void timerCallback() override;

    void repaintPianoRoll();

private:
    void updateEnablements();

    Audio2MidiAudioProcessor& mProcessor;

    State mPrevState = EmptyAudioAndMidiRegions;

    VisualizationPanel mVisualizationPanel;
    TranscriptionOptionsView mTranscriptionOptions;
    NoteOptionsView mNoteOptions;
    RhythmOptionsView mQuantizePanel;

    std::unique_ptr<juce::TextButton> mMuteButton;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> mMuteButtonAttachment;

    std::unique_ptr<juce::TextButton> mRecordButton;
    std::unique_ptr<juce::TextButton> mClearButton;

    std::unique_ptr<Knob> mMinNoteSlider;
    std::unique_ptr<Knob> mMaxNoteSlider;

    std::unique_ptr<ComboBox> mKey; // C, C#, D, D# ...
    std::unique_ptr<ComboBox> mMode; // Major, Minor, Chromatic

    // Eventually quantise functionality: need for time division, quantize force (0 - 100)
};

#endif // PluginMainView_h
