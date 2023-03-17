//
// Created by Damien Ronssin on 14.03.23.
//

#include "MinMaxNoteSlider.h"

MinMaxNoteSlider::MinMaxNoteSlider(std::atomic<int>& inAttachedMinValue,
                                   std::atomic<int>& inAttachedMaxValue,
                                   const std::function<void()>& inOnValueChange)
    : mAttachedMinValue(inAttachedMinValue)
    , mAttachedMaxValue(inAttachedMaxValue)
{
    mOnValueChanged = inOnValueChange;
    mSlider.setSliderStyle(juce::Slider::TwoValueHorizontal);
    mSlider.setRange(MIN_MIDI_NOTE, MAX_MIDI_NOTE, 1);
    mSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox, true, 0, 0);
    mSlider.onValueChange = [this]()
    {
        mAttachedMinValue.store(int(mSlider.getMinValue()));
        mAttachedMaxValue.store(int(mSlider.getMaxValue()));
        mOnValueChanged();
        repaint();
    };

    mSlider.setMinAndMaxValues(mAttachedMinValue.load(), mAttachedMaxValue.load());

    addAndMakeVisible(mSlider);
}

void MinMaxNoteSlider::resized()
{
    mSlider.setBounds(17, 0, 121, 17);
}

void MinMaxNoteSlider::paint(Graphics& g)
{
    g.setColour(juce::Colours::black);
    g.setFont(DROPDOWN_FONT);

    g.drawText(NoteUtils::midiNoteToStr(int(mSlider.getMinValue())),
               Rectangle<int>(0, 0, 22, 12),
               juce::Justification::centredLeft);

    g.drawText(NoteUtils::midiNoteToStr(int(mSlider.getMaxValue())),
               Rectangle<int>(133, 0, 22, 12),
               juce::Justification::centredRight);
}
