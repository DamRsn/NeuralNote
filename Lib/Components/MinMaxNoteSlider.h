//
// Created by Damien Ronssin on 14.03.23.
//

#ifndef TwoValueSlider_h
#define TwoValueSlider_h

#include <JuceHeader.h>
#include "NoteUtils.h"
#include "BasicPitchConstants.h"
#include "UIDefines.h"
#include "ParameterHelpers.h"

struct TwoValueAttachment {
    juce::Slider& slider;
    juce::RangedAudioParameter& minParameter;
    juce::RangedAudioParameter& maxParameter;
    std::unique_ptr<juce::ParameterAttachment> minAttach;
    std::unique_ptr<juce::ParameterAttachment> maxAttach;

    std::atomic<bool> minPerformingGesture = false;
    std::atomic<bool> maxPerformingGesture = false;

    TwoValueAttachment(juce::Slider& s, juce::RangedAudioParameter& min, juce::RangedAudioParameter& max)
        : slider(s)
        , minParameter(min)
        , maxParameter(max)
    {
        minAttach = std::make_unique<juce::ParameterAttachment>(
            minParameter, [this](float value) { slider.setMinValue(value); }, nullptr);

        maxAttach = std::make_unique<juce::ParameterAttachment>(
            maxParameter, [this](float value) { slider.setMaxValue(value); }, nullptr);

        slider.onDragStart = [this] {
            if (slider.getThumbBeingDragged() == 1 && !minPerformingGesture) {
                minAttach->beginGesture();
                minPerformingGesture = true;
            } else if (slider.getThumbBeingDragged() == 2 && !maxPerformingGesture) {
                maxAttach->beginGesture();
                maxPerformingGesture = true;
            }
        };

        slider.onDragEnd = [this] {
            if (minPerformingGesture) {
                minAttach->endGesture();
                minPerformingGesture = false;
            } else if (maxPerformingGesture) {
                maxAttach->endGesture();
                maxPerformingGesture = false;
            }
        };

        slider.onValueChange = [this] {
            if (slider.getThumbBeingDragged() == 1)
                minAttach->setValueAsPartOfGesture((float) slider.getMinValue());
            else if (slider.getThumbBeingDragged() == 2)
                maxAttach->setValueAsPartOfGesture((float) slider.getMaxValue());
        };

        minAttach->sendInitialUpdate();
        maxAttach->sendInitialUpdate();
    }
};

class MinMaxNoteSlider
    : public Component
    , public AudioProcessorValueTreeState::Listener
{
public:
    MinMaxNoteSlider(AudioProcessorValueTreeState& inAPVTS,
                     RangedAudioParameter& inMinValue,
                     RangedAudioParameter& inMaxValue);

    void resized() override;

    void paint(Graphics& g) override;

    void parameterChanged(const String& parameterID, float newValue) override;

private:
    juce::Slider mSlider;
    std::unique_ptr<TwoValueAttachment> mAttachment;
};

#endif // TwoValueSlider_h
