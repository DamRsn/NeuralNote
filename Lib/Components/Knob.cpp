//
// Created by Damien Ronssin on 07.03.23.
//

#include "Knob.h"

Knob::Knob(const std::string& inLabelText,
           double inMinRange,
           double inMaxRange,
           double inInterval,
           double inDefaultValue,
           bool inSetChangeNotificationOnlyOnRelease,
           std::atomic<float>& inAttachedValue,
           const std::function<void()>& inOnChangeAction,
           const std::string& inPostValue)
    : mAttachedValue(inAttachedValue)
    , mDefaultValue(inDefaultValue)
{
    mSlider.setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);
    mSlider.setRotaryParameters(5.0f * MathConstants<float>::pi / 4.0f,
                                11.0f * MathConstants<float>::pi / 4.0f,
                                true);

    mSlider.setRange(inMinRange, inMaxRange, inInterval);

    mSlider.setChangeNotificationOnlyOnRelease(inSetChangeNotificationOnlyOnRelease);

    mSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox, true, 40, 30);

    mLabel = inLabelText;
    mPostValueStr = inPostValue;

    mOnChangeAction = inOnChangeAction;

    mSlider.onValueChange = [this]()
    {
        mAttachedValue.store(static_cast<float>(mSlider.getValue()));
        mOnChangeAction();
        repaint();
    };

    mSlider.setDoubleClickReturnValue(true, mDefaultValue);
    mSlider.setValue(inAttachedValue.load());

    addAndMakeVisible(mSlider);

    mSlider.addMouseListener(this, false);
}

void Knob::resized()
{
    mSlider.setBounds(2, 0, 62, 62);
}

void Knob::paint(Graphics& g)
{
    float alpha = isEnabled() ? 1.0f : 0.5f;

    mSlider.setAlpha(alpha);
    g.setColour(juce::Colours::black.withAlpha(alpha));
    g.setFont(LABEL_FONT);

    if (!mIsMouseOver || !isEnabled())
    {
        g.drawMultiLineText(mLabel, 0, 73, 66, juce::Justification::centred, 0.0f);
    }
    else
    {
        String value = mSlider.getTextFromValue(mSlider.getValue());
        g.drawText(value + mPostValueStr,
                   juce::Rectangle<int>(0, 73, getWidth(), getHeight() - 67),
                   juce::Justification::centredTop);
    }
}

void Knob::mouseEnter(const MouseEvent& event)
{
    if (event.originalComponent == &mSlider)
    {
        mIsMouseOver = true;
        repaint();
    }
}

void Knob::mouseExit(const MouseEvent& event)
{
    if (event.originalComponent == &mSlider)
    {
        mIsMouseOver = false;
        repaint();
    }
}
