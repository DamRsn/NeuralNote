//
// Created by Damien Ronssin on 12.03.23.
//

#include "RhythmOptionsView.h"
#include "Audio2MidiMainView.h"

RhythmOptionsView::RhythmOptionsView(Audio2MidiAudioProcessor& processor)
    : mProcessor(processor)
{
    mTimeDivisionDropdown = std::make_unique<juce::ComboBox>("TimeDivisionDropDown");
    mTimeDivisionDropdown->setEditableText(false);
    mTimeDivisionDropdown->setJustificationType(juce::Justification::centredRight);
    mTimeDivisionDropdown->addItemList(RhythmUtils::TimeDivisionsStr, 1);
    mTimeDivisionDropdown->setSelectedItemIndex(
        mProcessor.getCustomParameters()->rhythmTimeDivision.load());
    mTimeDivisionDropdown->onChange = [this]()
    {
        mProcessor.getCustomParameters()->rhythmTimeDivision.store(
            mTimeDivisionDropdown->getSelectedItemIndex());
        _valueChanged();
    };
    addAndMakeVisible(*mTimeDivisionDropdown);

    mQuantization = std::make_unique<juce::Slider>();
    mQuantization->setSliderStyle(juce::Slider::LinearHorizontal);
    mQuantization->setRange(0, 100, 1);
    mQuantization->setDoubleClickReturnValue(true, 0);
    mQuantization->setTextBoxStyle(Slider::NoTextBox, true, 0.0f, 0.0f);

    mQuantization->onValueChange = [this]()
    {
        mProcessor.getCustomParameters()->rhythmQuantizationForce.store(
            static_cast<float>(mQuantization->getValue() / 100.0));
        _valueChanged();
    };
    addAndMakeVisible(*mQuantization);
}

void RhythmOptionsView::resized()
{
    mTimeDivisionDropdown->setBounds(148, 6 + mTopPad, 82, 19);
    mQuantization->setBounds(155, 36 + mTopPad, 100, 17);
}

void RhythmOptionsView::paint(Graphics& g)
{
    g.setColour(WHITE_TRANSPARENT);
    g.fillRoundedRectangle(0.0f,
                           static_cast<float>(mTopPad),
                           static_cast<float>(getWidth()),
                           static_cast<float>(getHeight() - mTopPad),
                           5.0f);

    float alpha = isEnabled() ? 1.0f : 0.5f;

    g.setColour(BLACK.withAlpha(alpha));
    g.setFont(TITLE_FONT);
    g.drawText("RHYTHM OPTIONS",
               Rectangle<int>(24, 0, 210, 17),
               juce::Justification::centredLeft);

    auto enable_rectangle = juce::Rectangle<int>(0, 0, 17, 17);
    if (isEnabled())
        g.fillRoundedRectangle(enable_rectangle.toFloat(), 4.0f);
    else
        g.drawRoundedRectangle(enable_rectangle.toFloat(), 4.0f, 1.0f);

    g.setFont(LABEL_FONT);
    g.drawText("TIME DIVISION",
               juce::Rectangle<int>(17, mTimeDivisionDropdown->getY(), 120, 17),
               juce::Justification::centredLeft);

    g.drawText("QUANTIZATION FORCE",
               juce::Rectangle<int>(17, mQuantization->getY(), 120, 17),
               juce::Justification::centredLeft);
}

void RhythmOptionsView::_valueChanged()
{
    if (mProcessor.getState() == PopulatedAudioAndMidiRegions)
    {
        mProcessor.updateTranscription();
        auto* main_view = dynamic_cast<Audio2MidiMainView*>(getParentComponent());

        if (main_view)
            main_view->repaintPianoRoll();
        else
            jassertfalse;
    }
}
