//
// Created by Damien Ronssin on 14.03.23.
//

#ifndef TwoValueSlider_h
#define TwoValueSlider_h

#include <JuceHeader.h>
#include "NoteUtils.h"
#include "BasicPitchConstants.h"
#include "UIDefines.h"

struct TwoValueAttachment {
    juce::Slider& slider;
    juce::RangedAudioParameter& minParameter;
    juce::RangedAudioParameter& maxParameter;
    std::unique_ptr<juce::ParameterAttachment> minAttach;
    std::unique_ptr<juce::ParameterAttachment> maxAttach;

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
            if (slider.getThumbBeingDragged() == 1)
                minAttach->beginGesture();
            else if (slider.getThumbBeingDragged() == 2)
                maxAttach->beginGesture();
        };

        slider.onDragEnd = [this] {
            if (slider.getThumbBeingDragged() == 1)
                minAttach->endGesture();
            else if (slider.getThumbBeingDragged() == 2)
                minAttach->endGesture();
        };

        slider.onValueChange = [this] {
            if (slider.getThumbBeingDragged() == 1)
                minAttach->setValueAsPartOfGesture(slider.getMinValue());
            else if (slider.getThumbBeingDragged() == 2)
                maxAttach->setValueAsPartOfGesture(slider.getMaxValue());
        };

        minAttach->sendInitialUpdate();
        maxAttach->sendInitialUpdate();
    }
};

class MinMaxNoteSlider : public Component
{
public:
    MinMaxNoteSlider(RangedAudioParameter& inMinValue, RangedAudioParameter& inMaxValue);

    void resized() override;

    void paint(Graphics& g) override;

private:
    juce::Slider mSlider;
    std::unique_ptr<TwoValueAttachment> mAttachment;

    //    std::atomic<int>& mAttachedMinValue;
    //    std::atomic<int>& mAttachedMaxValue;

    //        std::function<void()>
    //            mOnValueChanged;
};

#endif // TwoValueSlider_h
