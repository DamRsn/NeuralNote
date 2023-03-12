//
// Created by Damien Ronssin on 12.03.23.
//

#include "TranscriptionOptionsView.h"
TranscriptionOptionsView::TranscriptionOptionsView(Audio2MidiAudioProcessor& processor)
    : mProcessor(processor)
{
    mNoteSensibility =
        std::make_unique<Knob>("Note Sensibility",
                               0.05,
                               0.95,
                               0.01,
                               0.7,
                               false,
                               mProcessor.getCustomParameters()->noteSensibility);
    addAndMakeVisible(*mNoteSensibility);

    mSplitSensibility =
        std::make_unique<Knob>("Split Sensibility",
                               0.05,
                               0.95,
                               0.01,
                               0.5,
                               false,
                               mProcessor.getCustomParameters()->splitSensibility);
    addAndMakeVisible(*mSplitSensibility);

    mMinNoteDuration =
        std::make_unique<Knob>("Min Note Duration",
                               35,
                               580,
                               1,
                               125,
                               false,
                               mProcessor.getCustomParameters()->minNoteDurationMs);
    addAndMakeVisible(*mMinNoteDuration);

    mPitchBendDropDown = std::make_unique<juce::ComboBox>("Pitch Bend");
    mPitchBendDropDown->setEditableText(false);
    mPitchBendDropDown->setJustificationType(juce::Justification::centredRight);
    mPitchBendDropDown->addItemList(
        {"No Pitch Bend", "Single Pitch Bend", "Multi Pitch Bend"}, 1);
    mPitchBendDropDown->setSelectedId(1);
    mPitchBendDropDown->onChange = [this]()
    {
        mProcessor.getCustomParameters()->pitchBendMode.store(
            mPitchBendDropDown->getSelectedItemIndex());
    };
    addAndMakeVisible(*mPitchBendDropDown);

    setEnabled(false);
}

void TranscriptionOptionsView::resized()
{
    int button_y_start = 40;

    mNoteSensibility->setBounds(10, button_y_start, 75, 100);
    mSplitSensibility->setBounds(95, button_y_start, 75, 100);
    mMinNoteDuration->setBounds(180, button_y_start, 75, 100);
    mPitchBendDropDown->setBounds(84, 180, 114, 17);
}

void TranscriptionOptionsView::paint(Graphics& g)
{
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.fillRoundedRectangle(0.0f,
                           23.0f,
                           static_cast<float>(getWidth()),
                           static_cast<float>(getHeight() - 23),
                           5.0f);

    g.setColour(juce::Colours::black);
    g.setFont(12.0f);
    g.drawText("Transcription Options",
               Rectangle<int>(31, 0, 167, 20),
               juce::Justification::centred);

    auto enable_rectangle = juce::Rectangle<int>(10, 0, 17, 17);
    if (isEnabled())
        g.fillRect(enable_rectangle);
    else
        g.drawRect(enable_rectangle, 1.0f);

    g.setColour(juce::Colours::black);
    g.setFont(10.0f);
    g.drawText("Pitch Bend",
               juce::Rectangle<int>(11, 180, 66, 12),
               juce::Justification::centred);
}
