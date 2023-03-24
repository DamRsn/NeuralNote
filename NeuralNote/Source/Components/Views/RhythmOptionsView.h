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

class RhythmOptionsView : public Component
{
public:
    RhythmOptionsView(NeuralNoteAudioProcessor& processor);

    void resized() override;

    void paint(Graphics& g) override;

private:
    void _valueChanged();

    NeuralNoteAudioProcessor& mProcessor;

    std::unique_ptr<juce::ComboBox> mTimeDivisionDropdown;

    std::unique_ptr<QuantizeForceSlider> mQuantizationForceSlider;

    std::unique_ptr<juce::TextButton> mSnapMode;
};

#endif // RhythmOptionsView_h
