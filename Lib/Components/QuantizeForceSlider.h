//
// Created by Damien Ronssin on 20.03.23.
//

#ifndef HorizontalSlider_h
#define HorizontalSlider_h

#include <JuceHeader.h>
#include "NoteUtils.h"
#include "Constants.h"
#include "UIDefines.h"

class QuantizeForceSlider : public Component
{
public:
    QuantizeForceSlider(std::atomic<float>& inAttachedMaxValue,
                        const std::function<void()>& inOnValueChange);

    void resized() override;

    void paint(Graphics& g) override;

private:
    juce::Slider mSlider;

    std::atomic<float>& mAttachedValue;

    std::function<void()> mOnValueChanged;
};

#endif // HorizontalSlider_h
