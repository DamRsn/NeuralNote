//
// Created by Damien Ronssin on 14.03.23.
//

#ifndef TwoValueSlider_h
#define TwoValueSlider_h

#include <JuceHeader.h>
#include "NoteUtils.h"
#include "BasicPitchConstants.h"
#include "UIDefines.h"

class TwoValueAttachment : Slider::Listener
{
public:
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
            if (slider.getThumbBeingDragged() == 1) {
                minAttach->setValueAsPartOfGesture(static_cast<float>(slider.getMinValue()));
            } else if (slider.getThumbBeingDragged() == 2) {
                maxAttach->setValueAsPartOfGesture(static_cast<float>(slider.getMaxValue()));
            }
        };

        slider.addListener(this);

        // Important to set the max first, as the min value must be less than the max value
        maxAttach->sendInitialUpdate();
        minAttach->sendInitialUpdate();
    }

    ~TwoValueAttachment() override { slider.removeListener(this); }

private:
    void sliderValueChanged(Slider* s) override
    {
        if (s == &slider) {
            minAttach->setValueAsPartOfGesture((float) slider.getMinValue());
            maxAttach->setValueAsPartOfGesture((float) slider.getMaxValue());
        }
    }

    juce::Slider& slider;
    juce::RangedAudioParameter& minParameter;
    juce::RangedAudioParameter& maxParameter;
    std::unique_ptr<juce::ParameterAttachment> minAttach;
    std::unique_ptr<juce::ParameterAttachment> maxAttach;

    std::atomic<bool> minPerformingGesture = false;
    std::atomic<bool> maxPerformingGesture = false;
};

class MinMaxNoteSlider : public Component
{
public:
    MinMaxNoteSlider(RangedAudioParameter& inMinValue, RangedAudioParameter& inMaxValue);

    void resized() override;

    void paint(Graphics& g) override;

    void setTooltip(const String& inTooltip);

private:
    juce::Slider mSlider;
    std::unique_ptr<TwoValueAttachment> mAttachment;
};

#endif // TwoValueSlider_h
