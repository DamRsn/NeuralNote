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

class NeuralNoteMainView;

class NoteOptionsView : public Component

{
public:
    NoteOptionsView(NeuralNoteAudioProcessor& processor);

    void resized() override;

    void paint(Graphics& g) override;

private:
    void _valueChanged();

    NeuralNoteAudioProcessor& mProcessor;

    std::unique_ptr<MinMaxNoteSlider> mMinMaxNoteSlider;

    std::unique_ptr<juce::ComboBox> mKeyDropdown;
    std::unique_ptr<juce::ComboBox> mKeyType;
    std::unique_ptr<juce::ComboBox> mSnapMode;
};

#endif // NoteOptionsView_h
