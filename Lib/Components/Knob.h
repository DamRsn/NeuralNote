//
// Created by Damien Ronssin on 07.03.23.
//

#ifndef RotarySlider_h
#define RotarySlider_h

#include <JuceHeader.h>

#include "UIDefines.h"

class Knob : public juce::Component
{
public:
    Knob(const std::string& inLabelText,
         double inMinRange,
         double inMaxRange,
         double inInterval,
         double inDefaultValue,
         bool inSetChangeNotificationOnlyOnRelease,
         std::atomic<float>& inAttachedValue,
         const std::function<void()>& inOnChangeAction,
         const std::string& inPostValue = "");

    void resized() override;

    void paint(Graphics& g) override;

    void mouseEnter(const MouseEvent& event) override;

    void mouseExit(const MouseEvent& event) override;

private:
    juce::Slider mSlider;
    String mLabel;

    std::string mPostValueStr;
    const double mDefaultValue;
    std::atomic<float>& mAttachedValue;
    std::function<void()> mOnChangeAction;

    bool mIsMouseOver = false;
};

#endif // RotarySlider_h
