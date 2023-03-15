//
// Created by Damien Ronssin on 14.03.23.
//

#ifndef TwoValueSlider_h
#define TwoValueSlider_h

#include <JuceHeader.h>
#include "NoteUtils.h"
#include "Constants.h"
#include "UIDefines.h"

class MinMaxNoteSlider : public Component
{
public:
    MinMaxNoteSlider(std::atomic<int>& inAttachedMinValue,
                     std::atomic<int>& inAttachedMaxValue,
                     const std::function<void()>& inOnValueChange);

    void resized() override;

    void paint(Graphics& g) override;

private:
    juce::Slider mSlider;

    std::atomic<int>& mAttachedMinValue;
    std::atomic<int>& mAttachedMaxValue;

    std::function<void()> mOnValueChanged;
};

#endif // TwoValueSlider_h
