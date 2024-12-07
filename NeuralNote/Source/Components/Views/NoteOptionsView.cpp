//
// Created by Damien Ronssin on 12.03.23.
//

#include "NoteOptionsView.h"
#include "NeuralNoteMainView.h"

NoteOptionsView::NoteOptionsView(NeuralNoteAudioProcessor& processor)
    : mProcessor(processor)
{
    mEnableButton = std::make_unique<TextButton>("EnableNoteOptionsButton");
    mEnableButton->setButtonText("");
    mEnableButton->setClickingTogglesState(true);

    mEnableButton->setColour(TextButton::buttonColourId, Colours::white.withAlpha(0.2f));
    mEnableButton->setColour(TextButton::buttonOnColourId, BLACK);
    mEnableButton->setTooltip(NeuralNoteTooltips::sq_enable);

    mEnableAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(
        mProcessor.getAPVTS(), ParameterHelpers::getIdStr(ParameterHelpers::EnableNoteQuantizationId), *mEnableButton);
    addAndMakeVisible(mEnableButton.get());

    mMinMaxNoteSlider = std::make_unique<MinMaxNoteSlider>(*mProcessor.getParams()[ParameterHelpers::MinMidiNoteId],
                                                           *mProcessor.getParams()[ParameterHelpers::MaxMidiNoteId]);
    mMinMaxNoteSlider->setTooltip(NeuralNoteTooltips::sq_note_range);
    addAndMakeVisible(mMinMaxNoteSlider.get());

    mRootNoteDropdown = std::make_unique<ComboBox>("KeyRootNoteDropDown");
    mRootNoteDropdown->setEditableText(false);
    mRootNoteDropdown->setJustificationType(Justification::centredLeft);
    mRootNoteDropdown->addItemList(NoteUtils::RootNotesSharpStr, 1);
    mRootNoteDropdown->setTooltip(NeuralNoteTooltips::sq_root_note);
    mKeyAttachment = std::make_unique<ComboBoxParameterAttachment>(
        *mProcessor.getParams()[ParameterHelpers::KeyRootNoteId], *mRootNoteDropdown);
    addAndMakeVisible(mRootNoteDropdown.get());

    mKeyType = std::make_unique<ComboBox>("ScaleTypeDropDown");
    mKeyType->setEditableText(false);
    mKeyType->setJustificationType(Justification::centredLeft);
    mKeyType->addItemList(NoteUtils::ScaleTypesStr, 1);
    mKeyType->setTooltip(NeuralNoteTooltips::sq_scale_type);
    mKeyTypeAttachment =
        std::make_unique<ComboBoxParameterAttachment>(*mProcessor.getParams()[ParameterHelpers::KeyTypeId], *mKeyType);
    addAndMakeVisible(mKeyType.get());

    mSnapMode = std::make_unique<ComboBox>("SnapModeDropDown");
    mSnapMode->setEditableText(false);
    mSnapMode->setJustificationType(Justification::centredLeft);
    mSnapMode->addItemList(NoteUtils::SnapModesStr, 1);
    mSnapMode->setTooltip(NeuralNoteTooltips::sq_snap_mode);
    mSnapModeAttachment = std::make_unique<ComboBoxParameterAttachment>(
        *mProcessor.getParams()[ParameterHelpers::KeySnapModeId], *mSnapMode);
    addAndMakeVisible(mSnapMode.get());

    setSize(266, 139);

    mProcessor.getParams()[static_cast<size_t>(ParameterHelpers::EnableNoteQuantizationId)]->addListener(this);

    bool is_enabled = mProcessor.getParameterValue(ParameterHelpers::EnableNoteQuantizationId) > 0.5f;
    _enableView(is_enabled);
}

NoteOptionsView::~NoteOptionsView()
{
    mProcessor.getParams()[static_cast<size_t>(ParameterHelpers::EnableNoteQuantizationId)]->removeListener(this);
}

void NoteOptionsView::resized()
{
    mEnableButton->setBounds(0, 0, 18, 18);
    mMinMaxNoteSlider->setBounds(64, 17 + LEFT_SECTIONS_TOP_PAD, 189, 17);
    mRootNoteDropdown->setBounds(64, LEFT_SECTIONS_TOP_PAD + 46, 55, 17);
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

    float alpha = mIsViewEnabled && isEnabled() ? 1.0f : DISABLED_ALPHA;

    mMinMaxNoteSlider->setAlpha(alpha);
    g.setColour(BLACK.withAlpha(alpha));

    g.setFont(UIDefines::TITLE_FONT());
    g.drawText("SCALE QUANTIZE", Rectangle<int>(24, 0, 274, 17), Justification::centredLeft);

    g.setFont(UIDefines::LABEL_FONT());
    g.drawText("RANGE", Rectangle<int>(19, mMinMaxNoteSlider->getY(), 80, 17), Justification::centredLeft);

    g.drawText("KEY", Rectangle<int>(19, mRootNoteDropdown->getY(), 80, 17), Justification::centredLeft);

    g.drawText("SNAP MODE", Rectangle<int>(19, mSnapMode->getY(), 80, 17), Justification::centredLeft);
}

void NoteOptionsView::parameterValueChanged(int parameterIndex, float newValue)
{
    if (parameterIndex == static_cast<int>(ParameterHelpers::EnableNoteQuantizationId)) {
        bool enable = newValue > 0.5f;
        _enableView(enable);
    }
}

void NoteOptionsView::parameterGestureChanged(int parameterIndex, bool gestureIsStarting)
{
    ignoreUnused(parameterIndex, gestureIsStarting);
}

void NoteOptionsView::_enableView(bool inEnable)
{
    mIsViewEnabled = inEnable;
    mMinMaxNoteSlider->setEnabled(inEnable);
    mRootNoteDropdown->setEnabled(inEnable);
    mKeyType->setEnabled(inEnable);
    mSnapMode->setEnabled(inEnable);
    repaint();
}
