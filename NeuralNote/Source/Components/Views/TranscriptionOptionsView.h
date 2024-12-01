//
// Created by Damien Ronssin on 12.03.23.
//

#ifndef TranscriptionOptionsView_h
#define TranscriptionOptionsView_h

#include <JuceHeader.h>

#include "PluginProcessor.h"
#include "Knob.h"
#include "UIDefines.h"
#include "NeuralNoteTooltips.h"

class NeuralNoteMainView;

class TranscriptionOptionsView : public juce::Component
{
public:
    explicit TranscriptionOptionsView(NeuralNoteAudioProcessor& processor);

    void resized() override;

    void paint(Graphics& g) override;

private:
    NeuralNoteAudioProcessor& mProcessor;

    std::unique_ptr<Knob> mNoteSensibility;
    std::unique_ptr<Knob> mSplitSensibility;
    std::unique_ptr<Knob> mMinNoteDuration;

    std::unique_ptr<juce::ComboBox> mPitchBendDropDown;
    std::unique_ptr<juce::ComboBoxParameterAttachment> mPitchBendDropDownParameterAttachment;
};

#endif // TranscriptionOptionsView_h
