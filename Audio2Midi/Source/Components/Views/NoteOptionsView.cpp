//
// Created by Damien Ronssin on 12.03.23.
//

#include "NoteOptionsView.h"
NoteOptionsView::NoteOptionsView(Audio2MidiAudioProcessor& processor)
    : mProcessor(processor)
{
    mNoteMinDropDown = std::make_unique<juce::ComboBox>("MinNoteDropdown");
    mNoteMinDropDown->setEditableText(false);
    mNoteMinDropDown->setJustificationType(juce::Justification::centredRight);
    mNoteMinDropDown->addItemList({"A0", "A#0", "B0", "C1", "C#1"}, 1);
    mNoteMinDropDown->setSelectedId(1);
    mNoteMinDropDown->onChange = [this]()
    {
        mProcessor.getCustomParameters()->minMidiNote.store(
            mNoteMinDropDown->getSelectedItemIndex() + MIN_MIDI_NOTE);
    };
    addAndMakeVisible(*mNoteMinDropDown);

    mNoteMaxDropDown = std::make_unique<juce::ComboBox>("MaxNoteDropDown");
    mNoteMaxDropDown->setEditableText(false);
    mNoteMaxDropDown->setJustificationType(juce::Justification::centredRight);
    mNoteMaxDropDown->addItemList({"C8", "B7", "A#7", "G7", "F#7"}, 1);
    mNoteMaxDropDown->setSelectedId(1);
    mNoteMaxDropDown->onChange = [this]()
    {
        mProcessor.getCustomParameters()->maxMidiNote.store(
            MAX_MIDI_NOTE - mNoteMinDropDown->getSelectedItemIndex());
    };
    addAndMakeVisible(*mNoteMaxDropDown);

    mKeyDropdown = std::make_unique<juce::ComboBox>("KeyRootNoteDropDown");
    mKeyDropdown->setEditableText(false);
    mKeyDropdown->setJustificationType(juce::Justification::centredRight);
    mKeyDropdown->addItemList(
        {"A", "A#", "B", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#"}, 1);
    mKeyDropdown->setSelectedId(4);
    mKeyDropdown->onChange = [this]()
    {
        mProcessor.getCustomParameters()->keyRootNote.store(
            mKeyDropdown->getSelectedItemIndex());
    };
    addAndMakeVisible(*mKeyDropdown);

    mKeyType = std::make_unique<juce::ComboBox>("ScaleTypeDropDown");
    mKeyType->setEditableText(false);
    mKeyType->setJustificationType(juce::Justification::centredRight);
    mKeyType->addItemList({"Chromatic", "Major", "Minor"}, 1);
    mKeyType->setSelectedId(2);
    mKeyType->onChange = [this]() {
        mProcessor.getCustomParameters()->keyType.store(mKeyType->getSelectedItemIndex());
    };
    addAndMakeVisible(*mKeyType);

    setSize(266, 139);
}

void NoteOptionsView::resized()
{
    mNoteMinDropDown->setBounds(110, mTopPad + 18, 60, 17);
    mNoteMaxDropDown->setBounds(182, mTopPad + 18, 60, 17);
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
    g.setFont(12.0f);
    g.drawText(
        "Note Options", Rectangle<int>(31, 0, 167, 20), juce::Justification::centred);

    auto enable_rectangle = juce::Rectangle<int>(10, 0, 17, 17);
    if (isEnabled())
        g.fillRect(enable_rectangle);
    else
        g.drawRect(enable_rectangle, 1.0f);

    g.setColour(juce::Colours::black);
    g.setFont(10.0f);
    g.drawText("MIN / MAX NOTE",
               juce::Rectangle<int>(17, mNoteMinDropDown->getY(), 80, 17),
               juce::Justification::centredLeft);

    g.drawText("KEY",
               juce::Rectangle<int>(17, mKeyDropdown->getY(), 80, 17),
               juce::Justification::centredLeft);

    g.drawText("SNAP MODE",
               juce::Rectangle<int>(17, mKeyDropdown->getY() + 20, 80, 12),
               juce::Justification::centredLeft);
}
