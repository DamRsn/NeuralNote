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
    Knob(RangedAudioParameter& inParameter,
         const std::string& inLabelText,
         bool inSetChangeNotificationOnlyOnRelease,
         const std::string& inPostValue = "");

    void resized() override;

    void paint(Graphics& g) override;

    void mouseEnter(const MouseEvent& event) override;

    void mouseExit(const MouseEvent& event) override;

    Slider& getSlider() { return mSlider; }

    void setTooltip(const String& inTooltip);

private:
    juce::Slider mSlider;
    std::unique_ptr<juce::SliderParameterAttachment> mSliderParameterAttachment;
    String mLabel;

    std::string mPostValueStr;

    bool mIsMouseOver = false;
};

#endif // RotarySlider_h
