//
// Created by Damien Ronssin on 01.08.2024.
//

#ifndef NUMERICTEXTEDITOR_H
#define NUMERICTEXTEDITOR_H

#include "NnFonts.h"
#include "NnLook.h"
#include "PluginProcessor.h"
#include <type_traits>

#include <JuceHeader.h>

template <typename T, typename = std::enable_if_t<std::is_same_v<T, int> || std::is_same_v<T, double>>>
class NumericTextEditor
    : public TextEditor
    , public ValueTree::Listener
{
public:
    NumericTextEditor(NeuralNoteAudioProcessor* inProcessor,
                      const Identifier& inPropIdentifier,
                      int inMaxLength,
                      T inDefaultValue,
                      Justification inJustification,
                      const std::function<bool(String)>& inValidator,
                      const std::function<String(String)>& inCorrector)
        : mProcessor(inProcessor)
        , mIdentifier(inPropIdentifier)
    {
        setColour(textColourId, nn::colours::textFile);
        setColour(highlightedTextColourId, nn::colours::bgControlAlt);
        setColour(highlightColourId, nn::colours::accent);
        setColour(backgroundColourId, juce::Colours::transparentBlack);
        setColour(focusedOutlineColourId, juce::Colours::transparentBlack);
        setColour(outlineColourId, juce::Colours::transparentBlack);
        setColour(shadowColourId, juce::Colours::transparentBlack);
        setFont(nn::fonts::tempoValue());
        setJustification(inJustification);
        setReadOnly(false);

        String allowed_chars = std::is_same_v<T, int> ? String("0123456789") : String("0123456789.");
        setInputRestrictions(inMaxLength, allowed_chars);
        setClicksOutsideDismissVirtualKeyboard(true);
        setSelectAllWhenFocused(true);
        setTextToShowWhenEmpty(numberToStr(inDefaultValue), nn::colours::textDim);
        onReturnKey = [this] { giveAwayKeyboardFocus(); };
        onEscapeKey = [this] { giveAwayKeyboardFocus(); };

        onTextChange = [this, inPropIdentifier, inValidator]() {
            auto text = getText();

            if (inValidator(text)) {
                mProcessor->getValueTree().setPropertyExcludingListener(
                    this, inPropIdentifier, strToNumber(text), nullptr);
            }
        };

        onFocusLost = [this, inPropIdentifier, inValidator, inCorrector]() {
            setHighlightedRegion({0, 0});

            auto text = getText();

            if (!inValidator(text)) {
                auto corrected_text = inCorrector(text);
                jassert(inValidator(corrected_text));
                setText(corrected_text, true);
            }

            Desktop::getInstance().removeGlobalMouseListener(this);
        };

        setText(numberToStr(mProcessor->getValueTree().getProperty(mIdentifier)), false);

        mProcessor->addListenerToStateValueTree(this);
    }

    ~NumericTextEditor() override
    {
        mProcessor->removeListenerFromStateValueTree(this);
        Desktop::getInstance().removeGlobalMouseListener(this);
    }

    void focusGained(FocusChangeType cause) override
    {
        ignoreUnused(cause);
        Desktop::getInstance().addGlobalMouseListener(this);
    }

    static T strToNumber(const String& text)
    {
        if constexpr (std::is_same_v<T, int>) {
            return text.getIntValue();
        } else if constexpr (std::is_same_v<T, double>) {
            return text.getDoubleValue();
        }

        jassertfalse;
        return {};
    }

    static String numberToStr(T number)
    {
        if constexpr (std::is_same_v<T, int>) {
            return String(number);
        }

        if constexpr (std::is_same_v<T, double>) {
            if (juce::approximatelyEqual(number, std::round(number))) {
                return String(number, 0);
            }

            return String(number, 2);
        }

        return {};
    }

    void valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property) override
    {
        ignoreUnused(treeWhosePropertyHasChanged);
        if (property == mIdentifier) {
            setText(numberToStr(mProcessor->getValueTree().getProperty(mIdentifier)), false);
        }
    }

    void mouseDown(const MouseEvent& e) override
    {
        if (getScreenBounds().contains(e.getScreenPosition())) {
            // Delegate mouse clicks inside the editor to the TextEditor class to not break its functionality.
            TextEditor::mouseDown(e);
        } else {
            // Lose focus when mouse clicks occur outside the editor.
            giveAwayKeyboardFocus();
        }
    }

private:
    NeuralNoteAudioProcessor* mProcessor;

    const Identifier mIdentifier;
};

#endif //NUMERICTEXTEDITOR_H
