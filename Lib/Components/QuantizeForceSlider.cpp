//
// Created by Damien Ronssin on 20.03.23.
//

#include "QuantizeForceSlider.h"

QuantizeForceSlider::QuantizeForceSlider(RangedAudioParameter& inAttachedValue)
{
    mSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    mSlider.setRange(0, 100, 1);
    mSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox, true, 0, 0);
    mSlider.onValueChange = [this]() { repaint(); };

    mAttachment = std::make_unique<juce::SliderParameterAttachment>(inAttachedValue, mSlider);

    addAndMakeVisible(mSlider);
}

void QuantizeForceSlider::resized()
{
    mSlider.setBounds(0, 0, 128, 17);
}

void QuantizeForceSlider::paint(Graphics& g)
{
    g.setColour(juce::Colours::black);
    g.setFont(DROPDOWN_FONT);

    g.drawText(
        std::to_string(int(mSlider.getValue())), Rectangle<int>(133, 0, 23, 17), juce::Justification::centredRight);
}
