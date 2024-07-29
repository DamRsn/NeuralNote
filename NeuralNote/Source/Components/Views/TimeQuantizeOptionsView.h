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

class TimeQuantizeOptionsView
    : public Component
    , public AudioProcessorParameter::Listener
{
public:
    explicit TimeQuantizeOptionsView(NeuralNoteAudioProcessor& processor);

    ~TimeQuantizeOptionsView() override;

    void resized() override;

    void paint(Graphics& g) override;

private:
    void parameterValueChanged(int parameterIndex, float newValue) override;

    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override;

    void _setViewEnabled(bool inEnable);

    NeuralNoteAudioProcessor& mProcessor;

    std::unique_ptr<TextButton> mEnableButton;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> mEnableAttachment;

    std::unique_ptr<ComboBox> mTimeDivisionDropdown;
    std::unique_ptr<ComboBoxParameterAttachment> mTimeDivisionAttachment;

    std::unique_ptr<QuantizeForceSlider> mQuantizationForceSlider;

    bool mIsViewEnabled = false;
};

#endif // RhythmOptionsView_h
