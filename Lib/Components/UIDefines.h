//
// Created by Damien Ronssin on 12.03.23.
//

#ifndef NN_UIDEFINES_H
#define NN_UIDEFINES_H

#include <JuceHeader.h>
#include "BinaryData.h"

// Fonts

class UIFonts {
public:
    static UIFonts& get();

    Font TITLE_FONT() { return Font(pMONTSERRAT_BOLD).withPointHeight(18.0f); }
    Font LARGE_FONT() { return Font(pMONTSERRAT_BOLD).withPointHeight(20.0f); }
    Font LABEL_FONT() { return Font(pMONTSERRAT_SEMIBOLD).withPointHeight(10.0f); }
    Font DROPDOWN_FONT() { return Font(pMONTSERRAT_REGULAR).withPointHeight(10.0f); }
    Font BUTTON_FONT() { return Font(pMONTSERRAT_BOLD).withPointHeight(12.0f); }
    juce::Typeface::Ptr MONTSERRAT_BOLD() { return pMONTSERRAT_BOLD; }
    juce::Typeface::Ptr MONTSERRAT_SEMIBOLD() { return pMONTSERRAT_SEMIBOLD; }
    juce::Typeface::Ptr MONTSERRAT_REGULAR() { return pMONTSERRAT_REGULAR; }
protected:
    UIFonts() :
        pMONTSERRAT_BOLD(juce::Typeface::createSystemTypefaceFor(BinaryData::MontserratBold_ttf, BinaryData::MontserratBold_ttfSize)),
        pMONTSERRAT_SEMIBOLD(juce::Typeface::createSystemTypefaceFor(BinaryData::MontserratSemiBold_ttf, BinaryData::MontserratSemiBold_ttfSize)),
        pMONTSERRAT_REGULAR(juce::Typeface::createSystemTypefaceFor(BinaryData::MontserratRegular_ttf, BinaryData::MontserratRegular_ttfSize))
    { }
    static UIFonts* fts;

private:
    juce::Typeface::Ptr pMONTSERRAT_BOLD;
    juce::Typeface::Ptr pMONTSERRAT_SEMIBOLD;
    juce::Typeface::Ptr pMONTSERRAT_REGULAR;
};

// Colors
static const juce::Colour BLACK(juce::uint8(14), juce::uint8(14), juce::uint8(14));
static const juce::Colour WHITE_TRANSPARENT(juce::uint8(255), juce::uint8(253), juce::uint8(246), 0.7f);
static const juce::Colour WHITE_SOLID(juce::uint8(255), juce::uint8(253), juce::uint8(246), 1.0f);
static const juce::Colour WAVEFORM_COLOR(juce::uint8(255), juce::uint8(253), juce::uint8(246), 0.8f);
static const juce::Colour WAVEFORM_BG_COLOR(juce::uint8(0), juce::uint8(0), juce::uint8(0), 0.35f);
static const juce::Colour RECORD_RED(246, 89, 89);
static const juce::Colour PINK = juce::Colours::deeppink;
static const juce::Colour KNOB_GREY(218, 221, 217);

static const float DISABLED_ALPHA = 0.5f;

// Distances

static const int LEFT_SECTIONS_TOP_PAD = 24;

#endif //NN_UIDEFINES_H
