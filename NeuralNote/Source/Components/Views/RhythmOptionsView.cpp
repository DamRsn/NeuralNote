//
// Created by Damien Ronssin on 12.03.23.
//

#include "RhythmOptionsView.h"
#include "NeuralNoteMainView.h"

RhythmOptionsView::RhythmOptionsView(NeuralNoteAudioProcessor& processor)
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

    mQuantizationForceSlider = std::make_unique<QuantizeForceSlider>(
        mProcessor.getCustomParameters()->rhythmQuantizationForce,
        [this]() { _valueChanged(); });

    addAndMakeVisible(*mQuantizationForceSlider);
}

void RhythmOptionsView::resized()
{
    mTimeDivisionDropdown->setBounds(125, 13 + LEFT_SECTIONS_TOP_PAD, 75, 17);
    mQuantizationForceSlider->setBounds(94, 70 + LEFT_SECTIONS_TOP_PAD, 156, 17);
}

void RhythmOptionsView::paint(Graphics& g)
{
    g.setColour(WHITE_TRANSPARENT);
    g.fillRoundedRectangle(0.0f,
                           static_cast<float>(LEFT_SECTIONS_TOP_PAD),
                           static_cast<float>(getWidth()),
                           static_cast<float>(getHeight() - LEFT_SECTIONS_TOP_PAD),
                           5.0f);

    float alpha = isEnabled() ? 1.0f : 0.5f;

    mQuantizationForceSlider->setAlpha(alpha);
    g.setColour(BLACK.withAlpha(alpha));
    g.setFont(TITLE_FONT);
    g.drawText(
        "TIME QUANTIZE", Rectangle<int>(24, 0, 210, 17), juce::Justification::centredLeft);

    auto enable_rectangle = juce::Rectangle<int>(0, 0, 17, 17);
    if (isEnabled())
        g.fillRoundedRectangle(enable_rectangle.toFloat(), 4.0f);
    else
        g.drawRoundedRectangle(enable_rectangle.toFloat(), 4.0f, 1.0f);

    g.setFont(LABEL_FONT);

    g.drawText("TEMPO  " + mProcessor.getTempoStr(),
               juce::Rectangle<int>(19, LEFT_SECTIONS_TOP_PAD + 47, 75, 10),
               juce::Justification::centredLeft);

    g.drawText("TIME SIGNATURE  " + mProcessor.getTimeSignatureStr(),
               juce::Rectangle<int>(122, LEFT_SECTIONS_TOP_PAD + 47, 130, 10),
               juce::Justification::centredRight);

    g.drawText("TIME DIVISION",
               juce::Rectangle<int>(19, mTimeDivisionDropdown->getY(), 120, 17),
               juce::Justification::centredLeft);

    g.drawText("FORCE",
               juce::Rectangle<int>(19, mQuantizationForceSlider->getY(), 37, 17),
               juce::Justification::centredLeft);
}

void RhythmOptionsView::_valueChanged()
{
    if (mProcessor.getState() == PopulatedAudioAndMidiRegions)
    {
        mProcessor.updateTranscription();
        auto* main_view = dynamic_cast<NeuralNoteMainView*>(getParentComponent());

        if (main_view)
            main_view->repaintPianoRoll();
        else
            jassertfalse;
    }
}
