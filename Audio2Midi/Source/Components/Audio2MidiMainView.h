//
// Created by Damien Ronssin on 06.03.23.
//

#ifndef PluginMainView_h
#define PluginMainView_h

#include <JuceHeader.h>

#include "PluginProcessor.h"
#include "Knob.h"
#include "TranscriptionOptionsView.h"
#include "VisualizationPanel.h"
#include "NoteOptionsView.h"

class Audio2MidiMainView
    : public juce::Component
    , public juce::Slider::Listener
    , public juce::Timer
{
public:
    explicit Audio2MidiMainView(Audio2MidiAudioProcessor& processor);

    void resized() override;

    void paint(juce::Graphics& g) override;

    void timerCallback() override;

private:
    void sliderValueChanged(juce::Slider* inSliderPtr) override;

    void updateEnablements();

    Audio2MidiAudioProcessor& mProcessor;

    State mPrevState = EmptyAudioAndMidiRegions;

    VisualizationPanel mVisualizationPanel;
    TranscriptionOptionsView mTranscriptionOptions;
    NoteOptionsView mNoteOptions;

    std::unique_ptr<juce::Slider> mGainSlider;
    std::unique_ptr<juce::Button> mMuteButton;

    std::unique_ptr<juce::TextButton> mRecordButton;
    std::unique_ptr<juce::TextButton> mClearButton;

    std::unique_ptr<Knob> mMinNoteSlider;
    std::unique_ptr<Knob> mMaxNoteSlider;

    std::unique_ptr<ComboBox> mKey; // C, C#, D, D# ...
    std::unique_ptr<ComboBox> mMode; // Major, Minor, Chromatic

    // Eventually quantise functionality: need for time division, quantize force (0 - 100)
};

#endif // PluginMainView_h
