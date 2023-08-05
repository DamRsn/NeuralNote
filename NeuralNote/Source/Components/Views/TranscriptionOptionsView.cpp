//
// Created by Damien Ronssin on 12.03.23.
//

#include "TranscriptionOptionsView.h"
#include "NeuralNoteMainView.h"

TranscriptionOptionsView::TranscriptionOptionsView(NeuralNoteAudioProcessor& processor)
    : mProcessor(processor)
{
    mNoteSensibility =
        std::make_unique<Knob>(*mProcessor.mTree.getParameter("NOTE_SENSIBILITY"), "NOTE SENSIBILITY", false);
    addAndMakeVisible(*mNoteSensibility);

    mSplitSensibility =
        std::make_unique<Knob>(*mProcessor.mTree.getParameter("SPLIT_SENSIBILITY"), "SPLIT SENSIBILITY", false);
    addAndMakeVisible(*mSplitSensibility);

    mMinNoteDuration =
        std::make_unique<Knob>(*mProcessor.mTree.getParameter("MIN_NOTE_DURATION"), "MIN NOTE DURATION", false, " ms");
    addAndMakeVisible(*mMinNoteDuration);

    mPitchBendDropDown = std::make_unique<juce::ComboBox>("PITCH BEND");
    mPitchBendDropDown->setEditableText(false);
    mPitchBendDropDown->setJustificationType(juce::Justification::centredRight);
    mPitchBendDropDown->addItemList({"No Pitch Bend", "Single Pitch Bend"}, 1);
    mPitchBendDropDownParameterAttachment = std::make_unique<ComboBoxParameterAttachment>(
        *mProcessor.mTree.getParameter("PITCH_BEND_MODE"), *mPitchBendDropDown.get());

    addAndMakeVisible(*mPitchBendDropDown);

    mProcessor.mTree.addParameterListener("NOTE_SENSIBILITY", this);
    mProcessor.mTree.addParameterListener("SPLIT_SENSIBILITY", this);
    mProcessor.mTree.addParameterListener("MIN_NOTE_DURATION", this);
    mProcessor.mTree.addParameterListener("PITCH_BEND_MODE", this);
}

void TranscriptionOptionsView::resized()
{
    int button_y_start = 40;

    mNoteSensibility->setBounds(18, button_y_start, 66, 89);
    mSplitSensibility->setBounds(106, button_y_start, 66, 89);
    mMinNoteDuration->setBounds(193, button_y_start, 66, 89);
    mPitchBendDropDown->setBounds(100, 129 + LEFT_SECTIONS_TOP_PAD, 126, 17);
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
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (mProcessor.getState() == PopulatedAudioAndMidiRegions) {
        mProcessor.updateTranscription();
        auto* main_view = dynamic_cast<NeuralNoteMainView*>(getParentComponent());

        if (main_view)
            main_view->repaintPianoRoll();
        else
            jassertfalse;
    }
}
