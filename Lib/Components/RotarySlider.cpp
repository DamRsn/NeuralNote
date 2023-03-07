//
// Created by Damien Ronssin on 07.03.23.
//

#include "RotarySlider.h"

RotarySlider::RotarySlider(const std::string& inLabelText,
                           double inMinRange,
                           double inMaxRange,
                           double inInterval,
                           bool inSetChangeNotificationOnlyOnRelease)
{
    setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);
    setRotaryParameters(5.0f * MathConstants<float>::pi / 4.0f,
                        11.0f * MathConstants<float>::pi / 4.0f,
                        true);

    setRange(inMinRange, inMaxRange, inInterval);

    setChangeNotificationOnlyOnRelease(inSetChangeNotificationOnlyOnRelease);

    setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, true, 40, 30);
    setColour(juce::Slider::ColourIds::textBoxTextColourId, juce::Colours::black);

    addAndMakeVisible(mLabel);
    mLabel.setText(inLabelText, juce::NotificationType::dontSendNotification);
    mLabel.attachToComponent(this, false);
    mLabel.setJustificationType(juce::Justification::centredBottom);
    mLabel.setColour (juce::Label::textColourId, juce::Colours::orange);
}
