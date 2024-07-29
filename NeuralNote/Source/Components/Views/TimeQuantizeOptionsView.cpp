//
// Created by Damien Ronssin on 12.03.23.
//

#include "TimeQuantizeOptionsView.h"
#include "NeuralNoteMainView.h"

TimeQuantizeOptionsView::TimeQuantizeOptionsView(NeuralNoteAudioProcessor& processor)
    : mProcessor(processor)
{
    mEnableButton = std::make_unique<TextButton>("EnableTimeQuantizeOptionsButton");
    mEnableButton->setButtonText("");
    mEnableButton->setClickingTogglesState(true);

    mEnableButton->setColour(TextButton::buttonColourId, Colours::white.withAlpha(0.2f));
    mEnableButton->setColour(TextButton::buttonOnColourId, BLACK);

    mEnableAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(
        mProcessor.getAPVTS(), ParameterHelpers::getIdStr(ParameterHelpers::EnableTimeQuantizationId), *mEnableButton);
    addAndMakeVisible(mEnableButton.get());

    mTimeDivisionDropdown = std::make_unique<juce::ComboBox>("TimeDivisionDropDown");
    mTimeDivisionDropdown->setEditableText(false);
    mTimeDivisionDropdown->setJustificationType(juce::Justification::centredRight);
    mTimeDivisionDropdown->addItemList(TimeQuantizeUtils::TimeDivisionsStr, 1);
    mTimeDivisionAttachment = std::make_unique<juce::ComboBoxParameterAttachment>(
        *mProcessor.getParams()[ParameterHelpers::TimeDivisionId], *mTimeDivisionDropdown);
    addAndMakeVisible(*mTimeDivisionDropdown);

    mQuantizationForceSlider =
        std::make_unique<QuantizeForceSlider>(*mProcessor.getParams()[ParameterHelpers::QuantizationForceId]);

    addAndMakeVisible(*mQuantizationForceSlider);

    mProcessor.getParams()[static_cast<size_t>(ParameterHelpers::EnableTimeQuantizationId)]->addListener(this);
    bool is_view_enabled = mProcessor.getParameterValue(ParameterHelpers::EnableTimeQuantizationId) > 0.5f;
    _setViewEnabled(is_view_enabled);
}

TimeQuantizeOptionsView::~TimeQuantizeOptionsView()
{
    mProcessor.getParams()[static_cast<size_t>(ParameterHelpers::EnableTimeQuantizationId)]->removeListener(this);
}

void TimeQuantizeOptionsView::resized()
{
    mEnableButton->setBounds(0, 0, 18, 18);
    mTimeDivisionDropdown->setBounds(125, 13 + LEFT_SECTIONS_TOP_PAD, 75, 17);
    mQuantizationForceSlider->setBounds(94, 70 + LEFT_SECTIONS_TOP_PAD, 156, 17);
}

void TimeQuantizeOptionsView::paint(Graphics& g)
{
    g.setColour(WHITE_TRANSPARENT);
    g.fillRoundedRectangle(0.0f,
                           static_cast<float>(LEFT_SECTIONS_TOP_PAD),
                           static_cast<float>(getWidth()),
                           static_cast<float>(getHeight() - LEFT_SECTIONS_TOP_PAD),
                           5.0f);

    float alpha = isEnabled() && mIsViewEnabled ? 1.0f : DISABLED_ALPHA;

    mQuantizationForceSlider->setAlpha(alpha);
    g.setColour(BLACK.withAlpha(alpha));
    g.setFont(TITLE_FONT);
    g.drawText("TIME QUANTIZE", Rectangle<int>(24, 0, 210, 17), juce::Justification::centredLeft);

    g.setFont(LABEL_FONT);
    g.drawText("TEMPO  " + mProcessor.getTranscriptionManager()->getTimeQuantizeOptions().getTempoStr(),
               juce::Rectangle<int>(19, LEFT_SECTIONS_TOP_PAD + 47, 75, 10),
               juce::Justification::centredLeft);

    g.drawText("TIME SIGNATURE  "
                   + mProcessor.getTranscriptionManager()->getTimeQuantizeOptions().getTimeSignatureStr(),
               juce::Rectangle<int>(122, LEFT_SECTIONS_TOP_PAD + 47, 130, 10),
               juce::Justification::centredRight);

    g.drawText("TIME DIVISION",
               juce::Rectangle<int>(19, mTimeDivisionDropdown->getY(), 120, 17),
               juce::Justification::centredLeft);

    g.drawText(
        "FORCE", juce::Rectangle<int>(19, mQuantizationForceSlider->getY(), 37, 17), juce::Justification::centredLeft);
}

void TimeQuantizeOptionsView::parameterValueChanged(int parameterIndex, float newValue)
{
}

void TimeQuantizeOptionsView::parameterGestureChanged(int parameterIndex, bool gestureIsStarting)
{
}

void TimeQuantizeOptionsView::_setViewEnabled(bool inEnable)
{
    mIsViewEnabled = inEnable;
    mQuantizationForceSlider->setEnabled(inEnable);
    mTimeDivisionDropdown->setEnabled(inEnable);
}
