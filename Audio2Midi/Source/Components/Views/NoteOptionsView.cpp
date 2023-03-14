//
// Created by Damien Ronssin on 12.03.23.
//

#include "NoteOptionsView.h"

NoteOptionsView::NoteOptionsView(Audio2MidiAudioProcessor& processor)
    : mProcessor(processor)
{
    mMinMaxNoteSlider =
        std::make_unique<MinMaxNoteSlider>(mProcessor.getCustomParameters()->minMidiNote,
                                           mProcessor.getCustomParameters()->maxMidiNote);
    addAndMakeVisible(*mMinMaxNoteSlider);

    mKeyDropdown = std::make_unique<juce::ComboBox>("KeyRootNoteDropDown");
    mKeyDropdown->setEditableText(false);
    mKeyDropdown->setJustificationType(juce::Justification::centredLeft);
    mKeyDropdown->addItemList(NoteUtils::RootNotesSharpStr, 1);
    mKeyDropdown->setSelectedId(4);
    mKeyDropdown->onChange = [this]()
    {
        mProcessor.getCustomParameters()->keyRootNote.store(
            mKeyDropdown->getSelectedItemIndex());
    };
    addAndMakeVisible(*mKeyDropdown);

    mKeyType = std::make_unique<juce::ComboBox>("ScaleTypeDropDown");
    mKeyType->setEditableText(false);
    mKeyType->setJustificationType(juce::Justification::centredLeft);
    mKeyType->addItemList(NoteUtils::ScaleTypesStr, 1);
    mKeyType->setSelectedId(2);
    mKeyType->onChange = [this]() {
        mProcessor.getCustomParameters()->keyType.store(mKeyType->getSelectedItemIndex());
    };
    addAndMakeVisible(*mKeyType);

    setSize(266, 139);
}

void NoteOptionsView::resized()
{
    mMinMaxNoteSlider->setBounds(110, 22 + mTopPad, 154, 12);
    mKeyDropdown->setBounds(110, mTopPad + 42, 50, 17);
    mKeyType->setBounds(172, mTopPad + 42, 70, 17);
}

void NoteOptionsView::paint(Graphics& g)
{
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.fillRoundedRectangle(0.0f,
                           static_cast<float>(mTopPad),
                           static_cast<float>(getWidth()),
                           static_cast<float>(getHeight() - mTopPad),
                           5.0f);

    g.setColour(juce::Colours::black);
    g.setFont(TITLE_FONT);
    g.drawText(
        "NOTE OPTIONS", Rectangle<int>(31, 0, 167, 20), juce::Justification::centredLeft);

    auto enable_rectangle = juce::Rectangle<int>(10, 0, 17, 17);
    if (isEnabled())
        g.fillRect(enable_rectangle);
    else
        g.drawRect(enable_rectangle, 1.0f);

    g.setColour(juce::Colours::black);
    g.setFont(LABEL_FONT);
    g.drawText("RANGE",
               juce::Rectangle<int>(17, mMinMaxNoteSlider->getY(), 80, 17),
               juce::Justification::centredLeft);

    g.drawText("KEY",
               juce::Rectangle<int>(17, mKeyDropdown->getY(), 80, 17),
               juce::Justification::centredLeft);

    g.drawText("SNAP MODE",
               juce::Rectangle<int>(17, mKeyDropdown->getY() + 20, 80, 12),
               juce::Justification::centredLeft);
}
