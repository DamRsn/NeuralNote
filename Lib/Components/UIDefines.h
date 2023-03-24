//
// Created by Damien Ronssin on 12.03.23.
//

#ifndef NN_UIDEFINES_H
#define NN_UIDEFINES_H

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
static const juce::Colour BLACK(juce::uint8(14), juce::uint8(14), juce::uint8(14));
static const juce::Colour
    WHITE_TRANSPARENT(juce::uint8(255), juce::uint8(253), juce::uint8(246), 0.7f);
static const juce::Colour
    WHITE_SOLID(juce::uint8(255), juce::uint8(253), juce::uint8(246), 1.0f);
static const juce::Colour
    WAVEFORM_COLOR(juce::uint8(255), juce::uint8(253), juce::uint8(246), 0.8f);
static const juce::Colour
    WAVEFORM_BG_COLOR(juce::uint8(0), juce::uint8(0), juce::uint8(0), 0.35f);
static const juce::Colour RECORD_RED(246, 89, 89);
static const juce::Colour PINK = juce::Colours::deeppink;
static const juce::Colour KNOB_GREY(218, 221, 217);

static const float DISABLED_ALPHA = 0.5f;

// Distances

static const int LEFT_SECTIONS_TOP_PAD = 24;

#endif //NN_UIDEFINES_H
