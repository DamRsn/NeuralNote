//
// Created by Damien Ronssin on 12.03.23.
//

#include "TranscriptionOptionsView.h"
#include "NeuralNoteMainView.h"

TranscriptionOptionsView::TranscriptionOptionsView(NeuralNoteAudioProcessor& processor)
    : mProcessor(processor)
{
    mNoteSensibility =
        std::make_unique<Knob>(*mProcessor.getParams()[ParameterHelpers::NoteSensibilityId], "NOTE SENSIBILITY", false);
    addAndMakeVisible(*mNoteSensibility);

    mSplitSensibility = std::make_unique<Knob>(
        *mProcessor.getParams()[ParameterHelpers::SplitSensibilityId], "SPLIT SENSIBILITY", false);
    addAndMakeVisible(*mSplitSensibility);

    mMinNoteDuration = std::make_unique<Knob>(
        *mProcessor.getParams()[ParameterHelpers::MinimumNoteDurationId], "MIN NOTE DURATION", false, " ms");
    addAndMakeVisible(*mMinNoteDuration);

    mPitchBendDropDown = std::make_unique<juce::ComboBox>("PITCH BEND");
    mPitchBendDropDown->setEditableText(false);
    mPitchBendDropDown->setJustificationType(juce::Justification::centredLeft);
    mPitchBendDropDown->addItemList({"No Pitch Bend", "Single Pitch Bend"}, 1);
    mPitchBendDropDownParameterAttachment = std::make_unique<ComboBoxParameterAttachment>(
        *mProcessor.getParams()[ParameterHelpers::PitchBendModeId], *mPitchBendDropDown);

    addAndMakeVisible(*mPitchBendDropDown);

    mProcessor.mAPVTS.addParameterListener(ParameterHelpers::toIdStr(ParameterHelpers::NoteSensibilityId), this);
    mProcessor.mAPVTS.addParameterListener(ParameterHelpers::toIdStr(ParameterHelpers::SplitSensibilityId), this);
    mProcessor.mAPVTS.addParameterListener(ParameterHelpers::toIdStr(ParameterHelpers::MinimumNoteDurationId), this);
    mProcessor.mAPVTS.addParameterListener(ParameterHelpers::toIdStr(ParameterHelpers::PitchBendModeId), this);
}

void TranscriptionOptionsView::resized()
{
    int button_y_start = 40;

    mNoteSensibility->setBounds(18, button_y_start, 66, 89);
    mSplitSensibility->setBounds(106, button_y_start, 66, 89);
    mMinNoteDuration->setBounds(193, button_y_start, 66, 89);
    mPitchBendDropDown->setBounds(100, 129 + LEFT_SECTIONS_TOP_PAD, 154, 17);
}

void TranscriptionOptionsView::paint(Graphics& g)
{
    g.setColour(WHITE_TRANSPARENT);
    g.fillRoundedRectangle(0.0f,
                           LEFT_SECTIONS_TOP_PAD,
                           static_cast<float>(getWidth()),
                           static_cast<float>(getHeight() - LEFT_SECTIONS_TOP_PAD),
                           5.0f);

    float alpha = isEnabled() ? 1.0f : 0.5f;

    g.setColour(BLACK.withAlpha(alpha));
    g.setFont(TITLE_FONT);
    g.drawText("TRANSCRIPTION", Rectangle<int>(24, 0, 250, 17), juce::Justification::centredLeft);

    auto enable_rectangle = juce::Rectangle<int>(0, 0, 17, 17);
    if (isEnabled())
        g.fillRoundedRectangle(enable_rectangle.toFloat(), 4.0f);
    else
        g.drawRoundedRectangle(enable_rectangle.toFloat(), 4.0f, 1.0f);

    g.setFont(LABEL_FONT);
    g.drawText(
        "PITCH BEND", juce::Rectangle<int>(19, mPitchBendDropDown->getY(), 67, 17), juce::Justification::centredLeft);
}

void TranscriptionOptionsView::parameterChanged(const String& parameterID, float newValue)
{
    // Update atomic value in APVTS before calling updateTranscription
    mProcessor.mAPVTS.getRawParameterValue(parameterID)->store(newValue);

    MessageManager::callAsync([this]() {
        if (mProcessor.getState() == PopulatedAudioAndMidiRegions) {
            mProcessor.updateTranscription();
            auto* main_view = dynamic_cast<NeuralNoteMainView*>(getParentComponent());

            if (main_view)
                main_view->repaintPianoRoll();
            else
                jassertfalse;
        }
    });
}
