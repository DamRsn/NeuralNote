//
// Created by Damien Ronssin on 07.03.23.
//

#ifndef RotarySlider_h
#define RotarySlider_h

#include <JuceHeader.h>

class RotarySlider : public juce::Slider
{
public:
    RotarySlider(const std::string& inLabelText,
                 double inMinRange,
                 double inMaxRange,
                 double inInterval,
                 bool inSetChangeNotificationOnlyOnRelease);

    ~RotarySlider() = default;

private:
    juce::Label mLabel;
};

#endif // RotarySlider_h
