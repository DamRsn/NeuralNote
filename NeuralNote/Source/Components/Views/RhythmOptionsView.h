//
// Created by Damien Ronssin on 12.03.23.
//

#ifndef RhythmOptionsView_h
#define RhythmOptionsView_h

#include <JuceHeader.h>

#include "PluginProcessor.h"
#include "UIDefines.h"
#include "RhythmUtils.h"
#include "QuantizeForceSlider.h"

class NeuralNoteMainView;

class RhythmOptionsView
    : public Component
    , public AudioProcessorValueTreeState::Listener
{
public:
    explicit RhythmOptionsView(NeuralNoteAudioProcessor& processor);

    void resized() override;

    void paint(Graphics& g) override;

    void parameterChanged(const juce::String& parameterID, float newValue) override;

private:
    NeuralNoteAudioProcessor& mProcessor;

    std::unique_ptr<juce::ComboBox> mTimeDivisionDropdown;
    std::unique_ptr<juce::ComboBoxParameterAttachment> mTimeDivisionAttachment;

    std::unique_ptr<QuantizeForceSlider> mQuantizationForceSlider;
};

#endif // RhythmOptionsView_h
