//
// Created by Damien Ronssin on 12.03.23.
//

#include "NoteOptionsView.h"
#include "NeuralNoteMainView.h"

NoteOptionsView::NoteOptionsView(NeuralNoteAudioProcessor& processor)
    : mProcessor(processor)
{
    mMinMaxNoteSlider = std::make_unique<MinMaxNoteSlider>(*mProcessor.getParams()[ParameterHelpers::MinMidiNoteId],
                                                           *mProcessor.getParams()[ParameterHelpers::MaxMidiNoteId]);
    addAndMakeVisible(*mMinMaxNoteSlider);

    mKeyDropdown = std::make_unique<juce::ComboBox>("KeyRootNoteDropDown");
    mKeyDropdown->setEditableText(false);
    mKeyDropdown->setJustificationType(juce::Justification::centredLeft);
    mKeyDropdown->addItemList(NoteUtils::RootNotesSharpStr, 1);
    //    mKeyDropdown->onChange = [this]() {
    //        mProcessor.getCustomParameters()->keyRootNote.store(mKeyDropdown->getSelectedItemIndex());
    //        _valueChanged();
    //    };
    //    mKeyDropdown->setSelectedItemIndex(mProcessor.getCustomParameters()->keyRootNote.load());
    mKeyAttachment = std::make_unique<juce::ComboBoxParameterAttachment>(
        *mProcessor.getParams()[ParameterHelpers::KeyRootNoteId], *mKeyDropdown);
    addAndMakeVisible(*mKeyDropdown);

    mKeyType = std::make_unique<juce::ComboBox>("ScaleTypeDropDown");
    mKeyType->setEditableText(false);
    mKeyType->setJustificationType(juce::Justification::centredLeft);
    mKeyType->addItemList(NoteUtils::ScaleTypesStr, 1);
    //    mKeyType->onChange = [this]() {
    //        mProcessor.getCustomParameters()->keyType.store(mKeyType->getSelectedItemIndex());
    //        _valueChanged();
    //    };
    //    mKeyType->setSelectedItemIndex(mProcessor.getCustomParameters()->keyType.load());
    mKeyTypeAttachment = std::make_unique<juce::ComboBoxParameterAttachment>(
        *mProcessor.getParams()[ParameterHelpers::KeyTypeId], *mKeyType);
    addAndMakeVisible(*mKeyType);

    mSnapMode = std::make_unique<juce::ComboBox>("SnapModeDropDown");
    mSnapMode->setEditableText(false);
    mSnapMode->setJustificationType(juce::Justification::centredLeft);
    mSnapMode->addItemList(NoteUtils::SnapModesStr, 1);
    //    mSnapMode->onChange = [this]() {
    //        mProcessor.getCustomParameters()->keySnapMode.store(mSnapMode->getSelectedItemIndex());
    //        _valueChanged();
    //    };
    //    mSnapMode->setSelectedItemIndex(mProcessor.getCustomParameters()->keySnapMode.load());
    mSnapModeAttachment = std::make_unique<juce::ComboBoxParameterAttachment>(
        *mProcessor.getParams()[ParameterHelpers::KeySnapModeId], *mSnapMode);
    addAndMakeVisible(*mSnapMode);

    setSize(266, 139);

    mProcessor.mAPVTS.addParameterListener(ParameterHelpers::toIdStr(ParameterHelpers::MinMidiNoteId), this);
    mProcessor.mAPVTS.addParameterListener(ParameterHelpers::toIdStr(ParameterHelpers::MaxMidiNoteId), this);
    mProcessor.mAPVTS.addParameterListener(ParameterHelpers::toIdStr(ParameterHelpers::KeyRootNoteId), this);
    mProcessor.mAPVTS.addParameterListener(ParameterHelpers::toIdStr(ParameterHelpers::KeyTypeId), this);
    mProcessor.mAPVTS.addParameterListener(ParameterHelpers::toIdStr(ParameterHelpers::KeySnapModeId), this);
}

void NoteOptionsView::resized()
{
    mMinMaxNoteSlider->setBounds(64, 17 + LEFT_SECTIONS_TOP_PAD, 189, 17);
    mKeyDropdown->setBounds(64, LEFT_SECTIONS_TOP_PAD + 46, 55, 17);
    mKeyType->setBounds(124, LEFT_SECTIONS_TOP_PAD + 46, 129, 17);
    mSnapMode->setBounds(100, LEFT_SECTIONS_TOP_PAD + 75, 154, 17);
}

void NoteOptionsView::paint(Graphics& g)
{
    g.setColour(WHITE_TRANSPARENT);
    g.fillRoundedRectangle(0.0f,
                           static_cast<float>(LEFT_SECTIONS_TOP_PAD),
                           static_cast<float>(getWidth()),
                           static_cast<float>(getHeight() - LEFT_SECTIONS_TOP_PAD),
                           5.0f);

    float alpha = isEnabled() ? 1.0f : DISABLED_ALPHA;

    mMinMaxNoteSlider->setAlpha(alpha);
    g.setColour(BLACK.withAlpha(alpha));

    g.setFont(TITLE_FONT);
    g.drawText("SCALE QUANTIZE", Rectangle<int>(24, 0, 274, 17), juce::Justification::centredLeft);

    auto enable_rectangle = juce::Rectangle<int>(0, 0, 17, 17);
    if (isEnabled())
        g.fillRoundedRectangle(enable_rectangle.toFloat(), 4.0f);
    else
        g.drawRoundedRectangle(enable_rectangle.toFloat(), 4.0f, 1.0f);

    g.setFont(LABEL_FONT);
    g.drawText("RANGE", juce::Rectangle<int>(19, mMinMaxNoteSlider->getY(), 80, 17), juce::Justification::centredLeft);

    g.drawText("KEY", juce::Rectangle<int>(19, mKeyDropdown->getY(), 80, 17), juce::Justification::centredLeft);

    g.drawText("SNAP MODE", juce::Rectangle<int>(19, mSnapMode->getY(), 80, 17), juce::Justification::centredLeft);
}

void NoteOptionsView::parameterChanged(const String& parameterID, float newValue)
{
    mProcessor.mAPVTS.getRawParameterValue(parameterID)->store(newValue);

    MessageManager::callAsync([this]() {
        if (mProcessor.getState() == PopulatedAudioAndMidiRegions) {
            mProcessor.updatePostProcessing();

            auto* main_view = dynamic_cast<NeuralNoteMainView*>(getParentComponent());

            if (main_view)
                main_view->repaintPianoRoll();
            else
                jassertfalse;
        }
    });
}

//void NoteOptionsView::_valueChanged()
//{
//    if (mProcessor.getState() == PopulatedAudioAndMidiRegions) {
//        mProcessor.updatePostProcessing();
//
//        auto* main_view = dynamic_cast<NeuralNoteMainView*>(getParentComponent());
//
//        if (main_view)
//            main_view->repaintPianoRoll();
//        else
//            jassertfalse;
//    }
//}
