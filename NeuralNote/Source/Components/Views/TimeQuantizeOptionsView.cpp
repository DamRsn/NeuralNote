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

    mTimeDivisionDropdown = std::make_unique<ComboBox>("TimeDivisionDropDown");
    mTimeDivisionDropdown->setEditableText(false);
    mTimeDivisionDropdown->setJustificationType(Justification::centredRight);
    mTimeDivisionDropdown->addItemList(TimeQuantizeUtils::TimeDivisionsStr, 1);
    mTimeDivisionAttachment = std::make_unique<ComboBoxParameterAttachment>(
        *mProcessor.getParams()[ParameterHelpers::TimeDivisionId], *mTimeDivisionDropdown);
    addAndMakeVisible(*mTimeDivisionDropdown);

    mQuantizationForceSlider =
        std::make_unique<QuantizeForceSlider>(*mProcessor.getParams()[ParameterHelpers::QuantizationForceId]);

    addAndMakeVisible(*mQuantizationForceSlider);

    _setupTempoEditor();
    _setupTSEditors();

    mProcessor.getParams()[static_cast<size_t>(ParameterHelpers::EnableTimeQuantizationId)]->addListener(this);
    bool is_view_enabled = mProcessor.getParameterValue(ParameterHelpers::EnableTimeQuantizationId) > 0.5f;
    _setViewEnabled(is_view_enabled);

    mProcessor.addListenerToStateValueTree(this);
}

TimeQuantizeOptionsView::~TimeQuantizeOptionsView()
{
    mProcessor.getParams()[static_cast<size_t>(ParameterHelpers::EnableTimeQuantizationId)]->removeListener(this);
    mProcessor.getValueTree().removeListener(this);
}

void TimeQuantizeOptionsView::resized()
{
    mEnableButton->setBounds(0, 0, 18, 18);
    mTimeDivisionDropdown->setBounds(125, 13 + LEFT_SECTIONS_TOP_PAD, 75, 17);
    mQuantizationForceSlider->setBounds(94, 70 + LEFT_SECTIONS_TOP_PAD, 156, 17);
    mTempoEditor->setBounds(65, LEFT_SECTIONS_TOP_PAD + 44, 40, 14);
    mTimeSignatureNumEditor->setBounds(213, LEFT_SECTIONS_TOP_PAD + 44, 20, 14);
    mTimeSignatureDenomEditor->setBounds(238, LEFT_SECTIONS_TOP_PAD + 44, 20, 14);
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
    g.drawText("TIME QUANTIZE", Rectangle<int>(24, 0, 210, 17), Justification::centredLeft);

    g.setFont(LABEL_FONT);
    g.drawText("TEMPO", Rectangle<int>(19, LEFT_SECTIONS_TOP_PAD + 45, 50, 14), Justification::centredLeft);

    g.drawText("TIME SIGNATURE", Rectangle<int>(110, LEFT_SECTIONS_TOP_PAD + 45, 100, 14), Justification::centredRight);
    g.drawText("/", Rectangle<int>(233, LEFT_SECTIONS_TOP_PAD + 45, 5, 14), Justification::centred);

    g.drawText("TIME DIVISION", Rectangle<int>(19, mTimeDivisionDropdown->getY(), 120, 17), Justification::centredLeft);

    g.drawText("FORCE", Rectangle<int>(19, mQuantizationForceSlider->getY(), 37, 17), Justification::centredLeft);
}

void TimeQuantizeOptionsView::parameterValueChanged(int parameterIndex, float newValue)
{
    if (parameterIndex == static_cast<int>(ParameterHelpers::EnableTimeQuantizationId)) {
        _setViewEnabled(newValue > 0.5f);
    }
}

void TimeQuantizeOptionsView::parameterGestureChanged(int parameterIndex, bool gestureIsStarting)
{
    ignoreUnused(parameterIndex, gestureIsStarting);
}

void TimeQuantizeOptionsView::valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged,
                                                       const Identifier& property)
{
    if (property == NnId::TempoId) {
        mTempoEditor->setText(String(static_cast<float>(mProcessor.getValueTree().getProperty(NnId::TempoId))), false);
    }

    if (property == NnId::TimeSignatureNumeratorId) {
        mTimeSignatureNumEditor->setText(
            String(static_cast<int>(mProcessor.getValueTree().getProperty(NnId::TimeSignatureNumeratorId))), false);
    }

    if (property == NnId::TimeSignatureDenominatorId) {
        mTimeSignatureDenomEditor->setText(
            String(static_cast<int>(mProcessor.getValueTree().getProperty(NnId::TimeSignatureDenominatorId))), false);
    }
}

void TimeQuantizeOptionsView::_setViewEnabled(bool inEnable)
{
    mIsViewEnabled = inEnable;
    mQuantizationForceSlider->setEnabled(inEnable);
    mTimeDivisionDropdown->setEnabled(inEnable);
    repaint();
}

void TimeQuantizeOptionsView::_setupTempoEditor()
{
    mTempoEditor = std::make_unique<TextEditor>("TempoEditor");
    mTempoEditor->setColour(TextEditor::textColourId, BLACK);
    mTempoEditor->setColour(TextEditor::highlightedTextColourId, BLACK);
    mTempoEditor->setColour(TextEditor::backgroundColourId, TRANSPARENT);
    mTempoEditor->setColour(TextEditor::focusedOutlineColourId, TRANSPARENT);
    mTempoEditor->setColour(TextEditor::outlineColourId, TRANSPARENT);
    mTempoEditor->setColour(TextEditor::shadowColourId, TRANSPARENT);
    mTempoEditor->setFont(LABEL_FONT);
    mTempoEditor->setJustification(Justification::centredLeft);
    mTempoEditor->setReadOnly(false);
    mTempoEditor->setText(String(static_cast<float>(mProcessor.getValueTree().getProperty(NnId::TempoId))), false);
    mTempoEditor->setInputRestrictions(6, "0123456789.");
    mTempoEditor->setClicksOutsideDismissVirtualKeyboard(true);
    mTempoEditor->setSelectAllWhenFocused(true);
    mTempoEditor->setTextToShowWhenEmpty("120", BLACK);
    mTempoEditor->onReturnKey = [this] { mTempoEditor->giveAwayKeyboardFocus(); };
    mTempoEditor->onEscapeKey = [this] { mTempoEditor->giveAwayKeyboardFocus(); };

    auto is_tempo_str_valid = [](const String& tempo_str) {
        if (tempo_str.isEmpty()) {
            return false;
        }

        float tempo = tempo_str.getFloatValue();
        return tempo >= 20.0f && tempo <= 999.0f;
    };

    auto fix_tempo_str = [](const String& tempo_str) {
        return tempo_str.isEmpty() ? String("120") : String(jlimit(20.0f, 999.0f, tempo_str.getFloatValue()));
    };

    mTempoEditor->onFocusLost = [this, fix_tempo_str, is_tempo_str_valid] {
        mTempoEditor->setHighlightedRegion({0, 0});

        auto tempo_str = mTempoEditor->getText();

        if (!is_tempo_str_valid(tempo_str)) {
            auto new_tempo_str = fix_tempo_str(tempo_str);
            mTempoEditor->setText(new_tempo_str, true);
        }
    };

    mTempoEditor->onTextChange = [this, is_tempo_str_valid] {
        auto tempo_str = mTempoEditor->getText();

        // Only send change if valid tempo, otherwise wait.
        if (is_tempo_str_valid(tempo_str)) {
            auto tempo_val = tempo_str.getFloatValue();
            mProcessor.getValueTree().setPropertyExcludingListener(this, NnId::TempoId, tempo_val, nullptr);
        }
    };

    addAndMakeVisible(mTempoEditor.get());
}

void TimeQuantizeOptionsView::_setupTSEditors()
{
    // Numerator
    mTimeSignatureNumEditor = std::make_unique<TextEditor>("TSNumeratorEditor");
    mTimeSignatureNumEditor->setColour(TextEditor::textColourId, BLACK);
    mTimeSignatureNumEditor->setColour(TextEditor::highlightedTextColourId, BLACK);
    mTimeSignatureNumEditor->setColour(TextEditor::backgroundColourId, TRANSPARENT);
    mTimeSignatureNumEditor->setColour(TextEditor::focusedOutlineColourId, TRANSPARENT);
    mTimeSignatureNumEditor->setColour(TextEditor::outlineColourId, TRANSPARENT);
    mTimeSignatureNumEditor->setColour(TextEditor::shadowColourId, TRANSPARENT);
    mTimeSignatureNumEditor->setFont(LABEL_FONT);
    mTimeSignatureNumEditor->setJustification(Justification::centredRight);
    mTimeSignatureNumEditor->setReadOnly(false);
    mTimeSignatureNumEditor->setText(
        String(static_cast<float>(mProcessor.getValueTree().getProperty(NnId::TimeSignatureNumeratorId))), false);
    mTimeSignatureNumEditor->setInputRestrictions(2, "0123456789");
    mTimeSignatureNumEditor->setClicksOutsideDismissVirtualKeyboard(true);
    mTimeSignatureNumEditor->setSelectAllWhenFocused(true);
    mTimeSignatureNumEditor->setTextToShowWhenEmpty("4", BLACK);
    mTimeSignatureNumEditor->onReturnKey = [this] { mTimeSignatureNumEditor->giveAwayKeyboardFocus(); };
    mTimeSignatureNumEditor->onEscapeKey = [this] { mTimeSignatureNumEditor->giveAwayKeyboardFocus(); };

    auto is_valid_ts_num_str = [](const String& num_str) {
        if (num_str.isEmpty()) {
            return false;
        }

        int num_val = num_str.getIntValue();
        return num_val >= 1 && num_val <= 96;
    };

    auto fix_ts_num_str = [](const String& num_str) {
        return num_str.isEmpty() ? String("4") : String(jlimit(1, 96, num_str.getIntValue()));
    };

    mTimeSignatureNumEditor->onFocusLost = [this, is_valid_ts_num_str, fix_ts_num_str] {
        mTimeSignatureNumEditor->setHighlightedRegion({0, 0});

        auto num_ts_str = mTimeSignatureNumEditor->getText();
        if (!is_valid_ts_num_str(num_ts_str)) {
            mTimeSignatureNumEditor->setText(fix_ts_num_str(num_ts_str), true);
        }
    };

    mTimeSignatureNumEditor->onTextChange = [this, is_valid_ts_num_str] {
        auto num_ts_str = mTimeSignatureNumEditor->getText();

        if (is_valid_ts_num_str(num_ts_str)) {
            mProcessor.getValueTree().setPropertyExcludingListener(
                this, NnId::TimeSignatureNumeratorId, num_ts_str.getIntValue(), nullptr);
        }
    };

    addAndMakeVisible(mTimeSignatureNumEditor.get());

    // Denominator
    mTimeSignatureDenomEditor = std::make_unique<TextEditor>("TSDenominatorEditor");
    mTimeSignatureDenomEditor->setColour(TextEditor::textColourId, BLACK);
    mTimeSignatureDenomEditor->setColour(TextEditor::highlightedTextColourId, BLACK);
    mTimeSignatureDenomEditor->setColour(TextEditor::backgroundColourId, TRANSPARENT);
    mTimeSignatureDenomEditor->setColour(TextEditor::focusedOutlineColourId, TRANSPARENT);
    mTimeSignatureDenomEditor->setColour(TextEditor::outlineColourId, TRANSPARENT);
    mTimeSignatureDenomEditor->setColour(TextEditor::shadowColourId, TRANSPARENT);
    mTimeSignatureDenomEditor->setFont(LABEL_FONT);
    mTimeSignatureDenomEditor->setJustification(Justification::centredLeft);
    mTimeSignatureDenomEditor->setReadOnly(false);
    mTimeSignatureDenomEditor->setText(
        String(static_cast<float>(mProcessor.getValueTree().getProperty(NnId::TimeSignatureDenominatorId))), false);
    mTimeSignatureDenomEditor->setInputRestrictions(2, "0123456789");
    mTimeSignatureDenomEditor->setClicksOutsideDismissVirtualKeyboard(true);
    mTimeSignatureDenomEditor->setSelectAllWhenFocused(true);
    mTimeSignatureDenomEditor->setTextToShowWhenEmpty("4", BLACK);
    mTimeSignatureDenomEditor->onReturnKey = [this] { mTimeSignatureDenomEditor->giveAwayKeyboardFocus(); };
    mTimeSignatureDenomEditor->onEscapeKey = [this] { mTimeSignatureDenomEditor->giveAwayKeyboardFocus(); };

    auto is_valid_ts_denom_str = [](const String& denom_str) {
        if (denom_str.isEmpty()) {
            return false;
        }

        int denom_val = denom_str.getIntValue();

        // Closest power of 2
        int new_denom_val = static_cast<int>(std::exp2(std::round(std::log2(denom_val))));

        return denom_val == new_denom_val && denom_val >= 1 && denom_val <= 64;
    };

    auto fix_ts_denom_str = [](const String& denom_str) {
        if (denom_str.isEmpty()) {
            return String("4");
        }

        auto denom_val = denom_str.getIntValue();
        auto new_denom_val = static_cast<int>(std::exp2(std::round(std::log2(denom_val))));

        return String(jlimit(1, 64, new_denom_val));
    };

    mTimeSignatureDenomEditor->onFocusLost = [this, is_valid_ts_denom_str, fix_ts_denom_str] {
        mTimeSignatureDenomEditor->setHighlightedRegion({0, 0});

        auto denom_str = mTimeSignatureDenomEditor->getText();
        if (!is_valid_ts_denom_str(denom_str)) {
            mTimeSignatureDenomEditor->setText(fix_ts_denom_str(denom_str), true);
        }
    };

    mTimeSignatureDenomEditor->onTextChange = [this, is_valid_ts_denom_str] {
        auto num_str = mTimeSignatureDenomEditor->getText();

        if (is_valid_ts_denom_str(num_str)) {
            mProcessor.getValueTree().setPropertyExcludingListener(
                this, NnId::TimeSignatureDenominatorId, num_str.getIntValue(), nullptr);
        }
    };

    addAndMakeVisible(mTimeSignatureDenomEditor.get());
}