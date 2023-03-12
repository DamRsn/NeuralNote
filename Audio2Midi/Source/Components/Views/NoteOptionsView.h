//
// Created by Damien Ronssin on 12.03.23.
//

#ifndef NoteOptionsView_h
#define NoteOptionsView_h

#include <JuceHeader.h>

#include "PluginProcessor.h"

class NoteOptionsView : public Component
{
public:
    NoteOptionsView(Audio2MidiAudioProcessor& processor);

    void resized() override;

    void paint(Graphics& g) override;

private:
    const int mTopPad = 23;
    Audio2MidiAudioProcessor& mProcessor;

    std::unique_ptr<juce::ComboBox> mNoteMinDropDown;
    std::unique_ptr<juce::ComboBox> mNoteMaxDropDown;

    std::unique_ptr<juce::ComboBox> mKeyDropdown;
    std::unique_ptr<juce::ComboBox> mKeyType;

    std::unique_ptr<juce::TextButton> mSnapMode;
};

#endif // NoteOptionsView_h