//
// Created by Damien Ronssin on 12.03.23.
//

#ifndef RhythmOptionsView_h
#define RhythmOptionsView_h

#include <JuceHeader.h>

#include "PluginProcessor.h"
#include "UIDefines.h"
#include "TimeQuantizeUtils.h"
#include "QuantizeForceSlider.h"

class NeuralNoteMainView;

class RhythmOptionsView : public Component
{
public:
    explicit RhythmOptionsView(NeuralNoteAudioProcessor& processor);

    void resized() override;

    void paint(Graphics& g) override;

private:
    NeuralNoteAudioProcessor& mProcessor;

    std::unique_ptr<juce::ComboBox> mTimeDivisionDropdown;
    std::unique_ptr<juce::ComboBoxParameterAttachment> mTimeDivisionAttachment;

    std::unique_ptr<QuantizeForceSlider> mQuantizationForceSlider;
};

#endif // RhythmOptionsView_h
