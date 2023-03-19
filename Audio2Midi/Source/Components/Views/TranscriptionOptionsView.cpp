//
// Created by Damien Ronssin on 12.03.23.
//

#include "TranscriptionOptionsView.h"
#include "Audio2MidiMainView.h"

TranscriptionOptionsView::TranscriptionOptionsView(Audio2MidiAudioProcessor& processor)
    : mProcessor(processor)
{
    mNoteSensibility =
        std::make_unique<Knob>("NOTE SENSIBILITY",
                               0.05,
                               0.95,
                               0.01,
                               0.7,
                               false,
                               mProcessor.getCustomParameters()->noteSensibility,
                               [this]() { _valueChanged(); });

    addAndMakeVisible(*mNoteSensibility);

    mSplitSensibility =
        std::make_unique<Knob>("SPLIT SENSIBILITY",
                               0.05,
                               0.95,
                               0.01,
                               0.5,
                               false,
                               mProcessor.getCustomParameters()->splitSensibility,
                               [this]() { _valueChanged(); });

    addAndMakeVisible(*mSplitSensibility);

    mMinNoteDuration =
        std::make_unique<Knob>("MIN NOTE DURATION",
                               35,
                               580,
                               1,
                               125,
                               false,
                               mProcessor.getCustomParameters()->minNoteDurationMs,
                               [this]() { _valueChanged(); });

    addAndMakeVisible(*mMinNoteDuration);

    mPitchBendDropDown = std::make_unique<juce::ComboBox>("PITCH BEND");
    mPitchBendDropDown->setEditableText(false);
    mPitchBendDropDown->setJustificationType(juce::Justification::centredRight);
    mPitchBendDropDown->addItemList({"No Pitch Bend", "Single Pitch Bend"}, 1);
    mPitchBendDropDown->setSelectedItemIndex(0);
    mPitchBendDropDown->onChange = [this]()
    {
        mProcessor.getCustomParameters()->pitchBendMode.store(
            mPitchBendDropDown->getSelectedItemIndex());
    };
    addAndMakeVisible(*mPitchBendDropDown);
}

void TranscriptionOptionsView::resized()
{
    int button_y_start = 40;

    mNoteSensibility->setBounds(10, button_y_start, 66, 89);
    mSplitSensibility->setBounds(95, button_y_start, 66, 89);
    mMinNoteDuration->setBounds(180, button_y_start, 66, 89);
    mPitchBendDropDown->setBounds(110, 153 + 23, 116, 17);
}

void TranscriptionOptionsView::paint(Graphics& g)
{
    g.setColour(WHITE_TRANSPARENT);
    g.fillRoundedRectangle(0.0f,
                           23.0f,
                           static_cast<float>(getWidth()),
                           static_cast<float>(getHeight() - 23),
                           5.0f);

    float alpha = isEnabled() ? 1.0f : 0.5f;

    g.setColour(BLACK.withAlpha(alpha));
    g.setFont(TITLE_FONT);
    g.drawText("TRANSCRIPTION OPTIONS",
               Rectangle<int>(24, 0, 250, 17),
               juce::Justification::centred);

    auto enable_rectangle = juce::Rectangle<int>(0, 0, 17, 17);
    if (isEnabled())
        g.fillRoundedRectangle(enable_rectangle.toFloat(), 4.0f);
    else
        g.drawRoundedRectangle(enable_rectangle.toFloat(), 4.0f, 1.0f);

    g.setFont(LABEL_FONT);
    g.drawText("PITCH BEND",
               juce::Rectangle<int>(11, 153 + 23, 67, 12),
               juce::Justification::centred);
}

void TranscriptionOptionsView::_valueChanged()
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
