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
    mTimeDivisionDropdown->setSelectedItemIndex(
        mProcessor.getCustomParameters()->rhythmTimeDivision.load());
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
}

void RhythmOptionsView::resized()
{
    mTimeDivisionDropdown->setBounds(148, 6 + mTopPad, 82, 19);
    mQuantization->setBounds(128, 36 + mTopPad, 72, 17);
}

void RhythmOptionsView::paint(Graphics& g)
{
    g.setColour(WHITE_BG);
    g.fillRoundedRectangle(0.0f,
                           static_cast<float>(mTopPad),
                           static_cast<float>(getWidth()),
                           static_cast<float>(getHeight() - mTopPad),
                           5.0f);

    g.setColour(FONT_BLACK);
    g.setFont(TITLE_FONT);
    g.drawText("RHYTHM OPTIONS",
               Rectangle<int>(24, 0, 210, 17),
               juce::Justification::centredLeft);

    auto enable_rectangle = juce::Rectangle<int>(0, 0, 17, 17);
    if (isEnabled())
        g.fillRoundedRectangle(enable_rectangle.toFloat(), 4.0f);
    else
        g.drawRoundedRectangle(enable_rectangle.toFloat(), 4.0f, 1.0f);

    g.setColour(FONT_BLACK);
    g.setFont(LABEL_FONT);
    g.drawText("TIME DIVISION",
               juce::Rectangle<int>(17, mTimeDivisionDropdown->getY(), 120, 17),
               juce::Justification::centredLeft);

    g.drawText("QUANTIZATION FORCE",
               juce::Rectangle<int>(17, mQuantization->getY(), 120, 17),
               juce::Justification::centredLeft);
}
