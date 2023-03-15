//
// Created by Damien Ronssin on 12.03.23.
//

#ifndef AUDIO2MIDIPLUGIN_UIDEFINES_H
#define AUDIO2MIDIPLUGIN_UIDEFINES_H

#include <JuceHeader.h>
#include "BinaryData.h"

// Fonts
const juce::Typeface::Ptr MONTSERRAT_BOLD = juce::Typeface::createSystemTypefaceFor(
    BinaryData::MontserratBold_ttf, BinaryData::MontserratBold_ttfSize);

const juce::Typeface::Ptr MONTSERRAT_SEMIBOLD = juce::Typeface::createSystemTypefaceFor(
    BinaryData::MontserratSemiBold_ttf, BinaryData::MontserratSemiBold_ttfSize);

const juce::Typeface::Ptr MONTSERRAT_REGULAR = juce::Typeface::createSystemTypefaceFor(
    BinaryData::MontserratRegular_ttf, BinaryData::MontserratRegular_ttfSize);

const Font TITLE_FONT = Font(MONTSERRAT_BOLD).withPointHeight(18.0f);
const Font LARGE_FONT = Font(MONTSERRAT_BOLD).withPointHeight(20.0f);
const Font LABEL_FONT = Font(MONTSERRAT_SEMIBOLD).withPointHeight(10.0f);
const Font DROPDOWN_FONT = Font(MONTSERRAT_REGULAR).withPointHeight(10.0f);
const Font BUTTON_FONT = Font(MONTSERRAT_BOLD).withPointHeight(12.0f);

// Colors
static const juce::Colour FONT_BLACK(juce::uint8(14), juce::uint8(14), juce::uint8(14));
static const juce::Colour
    WHITE_BG(juce::uint8(255), juce::uint8(253), juce::uint8(246), 0.8f);
static const juce::Colour
    WAVEFORM_COLOR(juce::uint8(255), juce::uint8(253), juce::uint8(246), 0.8f);
static const juce::Colour
    WAVEFORM_BG_COLOR(juce::uint8(0), juce::uint8(0), juce::uint8(0), 0.35f);

#endif //AUDIO2MIDIPLUGIN_UIDEFINES_H
