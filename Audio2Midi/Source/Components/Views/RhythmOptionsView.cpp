//
// Created by Damien Ronssin on 12.03.23.
//

#include "RhythmOptionsView.h"

RhythmOptionsView::RhythmOptionsView(Audio2MidiAudioProcessor& processor)
    : mProcessor(processor)
{
    mTimeDivisionDropdown = std::make_unique<juce::ComboBox>("TimeDivisionDropDown");
    mTimeDivisionDropdown->setEditableText(false);
    mTimeDivisionDropdown->setJustificationType(juce::Justification::centredRight);
    mTimeDivisionDropdown->addItemList({"1/1",
                                        "1/2",
                                        "1/3",
                                        "1/4",
                                        "1/6",
                                        "1/8",
                                        "1/12",
                                        "1/16",
                                        "1/24",
                                        "1/32",
                                        "1/48",
                                        "1/64"},
                                       1);
    mTimeDivisionDropdown->setSelectedId(4);
    mTimeDivisionDropdown->onChange = [this]()
    {
        mProcessor.getCustomParameters()->rhythmTimeDivision.store(
            mTimeDivisionDropdown->getSelectedItemIndex());
    };
    addAndMakeVisible(*mTimeDivisionDropdown);

    mQuantization = std::make_unique<juce::Slider>();
    mQuantization->setSliderStyle(juce::Slider::LinearHorizontal);
    mQuantization->setRange(0, 100, 1);
    mQuantization->setDoubleClickReturnValue(true, 0);
    addAndMakeVisible(*mQuantization);

    setSize(266, 83);
}

void RhythmOptionsView::resized()
{
    mTimeDivisionDropdown->setBounds(148, 6 + mTopPad, 82, 19);
    mQuantization->setBounds(128, 36 + mTopPad, 72, 17);
}

void RhythmOptionsView::paint(Graphics& g)
{
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.fillRoundedRectangle(0.0f,
                           static_cast<float>(mTopPad),
                           static_cast<float>(getWidth()),
                           static_cast<float>(getHeight() - mTopPad),
                           5.0f);

    g.setColour(juce::Colours::black);
    g.setFont(TITLE_FONT);
    g.drawText("RHYTHM OPTIONS",
               Rectangle<int>(31, 0, 167, 20),
               juce::Justification::centredLeft);

    auto enable_rectangle = juce::Rectangle<int>(10, 0, 17, 17);
    if (isEnabled())
        g.fillRect(enable_rectangle);
    else
        g.drawRect(enable_rectangle, 1.0f);

    g.setColour(juce::Colours::black);
    g.setFont(LABEL_FONT);
    g.drawText("TIME DIVISION",
               juce::Rectangle<int>(17, mTimeDivisionDropdown->getY(), 120, 17),
               juce::Justification::centredLeft);

    g.drawText("QUANTIZATION FORCE",
               juce::Rectangle<int>(17, mQuantization->getY(), 120, 17),
               juce::Justification::centredLeft);
}
