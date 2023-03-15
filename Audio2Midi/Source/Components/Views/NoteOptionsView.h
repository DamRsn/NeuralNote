//
// Created by Damien Ronssin on 12.03.23.
//

#ifndef NoteOptionsView_h
#define NoteOptionsView_h

#include <JuceHeader.h>

#include "PluginProcessor.h"
#include "NoteOptions.h"
#include "UIDefines.h"
#include "NoteUtils.h"
#include "MinMaxNoteSlider.h"

class Audio2MidiMainView;

class NoteOptionsView
    : public Component
    , public juce::Slider::Listener
    , public juce::ComboBox::Listener
{
public:
    NoteOptionsView(Audio2MidiAudioProcessor& processor);

    void resized() override;

    void paint(Graphics& g) override;

    void sliderValueChanged(Slider* slider) override;

    void comboBoxChanged(ComboBox* comboBoxThatHasChanged) override;

private:
    void _valueChanged();

    const int mTopPad = 23;
    Audio2MidiAudioProcessor& mProcessor;

    std::unique_ptr<MinMaxNoteSlider> mMinMaxNoteSlider;

    std::unique_ptr<juce::ComboBox> mKeyDropdown;
    std::unique_ptr<juce::ComboBox> mKeyType;
    std::unique_ptr<juce::ComboBox> mSnapMode;
};

#endif // NoteOptionsView_h
