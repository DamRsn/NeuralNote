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
    explicit NoteOptionsView(NeuralNoteAudioProcessor& processor);

    void resized() override;

    void paint(Graphics& g) override;

private:
    NeuralNoteAudioProcessor& mProcessor;

    std::unique_ptr<MinMaxNoteSlider> mMinMaxNoteSlider;

    std::unique_ptr<juce::ComboBox> mKeyDropdown;
    std::unique_ptr<juce::ComboBoxParameterAttachment> mKeyAttachment;

    std::unique_ptr<juce::ComboBox> mKeyType;
    std::unique_ptr<juce::ComboBoxParameterAttachment> mKeyTypeAttachment;

    std::unique_ptr<juce::ComboBox> mSnapMode;
    std::unique_ptr<juce::ComboBoxParameterAttachment> mSnapModeAttachment;
};

#endif // NoteOptionsView_h
