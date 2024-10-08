//
// Created by Damien Ronssin on 12.03.23.
//

#ifndef NN_UIDEFINES_H
#define NN_UIDEFINES_H

#include <JuceHeader.h>
#include "BinaryData.h"

// Fonts
const Typeface::Ptr MONTSERRAT_BOLD =
    Typeface::createSystemTypefaceFor(BinaryData::MontserratBold_ttf, BinaryData::MontserratBold_ttfSize);

const Typeface::Ptr MONTSERRAT_SEMIBOLD =
    Typeface::createSystemTypefaceFor(BinaryData::MontserratSemiBold_ttf, BinaryData::MontserratSemiBold_ttfSize);

const Typeface::Ptr MONTSERRAT_REGULAR =
    Typeface::createSystemTypefaceFor(BinaryData::MontserratRegular_ttf, BinaryData::MontserratRegular_ttfSize);

const Font TITLE_FONT = Font(FontOptions(MONTSERRAT_BOLD)).withPointHeight(18.0f);
const Font LARGE_FONT = Font(FontOptions(MONTSERRAT_BOLD)).withPointHeight(20.0f);
const Font LABEL_FONT = Font(FontOptions(MONTSERRAT_SEMIBOLD)).withPointHeight(10.0f);
const Font DROPDOWN_FONT = Font(FontOptions(MONTSERRAT_REGULAR)).withPointHeight(10.0f);
const Font BUTTON_FONT = Font(FontOptions(MONTSERRAT_BOLD)).withPointHeight(12.0f);

// Colors
static const Colour BLACK(static_cast<uint8>(14), static_cast<uint8>(14), static_cast<uint8>(14));
static const Colour WHITE_TRANSPARENT(static_cast<uint8>(255), static_cast<uint8>(253), static_cast<uint8>(246), 0.7f);
static const Colour WHITE_SOLID(static_cast<uint8>(255), static_cast<uint8>(253), static_cast<uint8>(246), 1.0f);
static const Colour WAVEFORM_COLOR(static_cast<uint8>(255), static_cast<uint8>(253), static_cast<uint8>(246), 0.8f);
static const Colour WAVEFORM_BG_COLOR(static_cast<uint8>(0), static_cast<uint8>(0), static_cast<uint8>(0), 0.35f);
static const Colour RECORD_RED(246, 89, 89);
static const Colour PINK = Colours::deeppink;
static const Colour KNOB_GREY(218, 221, 217);
static const Colour TRANSPARENT(static_cast<uint8>(0), static_cast<uint8>(0), static_cast<uint8>(0), 0.0f);

static constexpr float DISABLED_ALPHA = 0.5f;

// Distances

static constexpr int LEFT_SECTIONS_TOP_PAD = 24;

#endif //NN_UIDEFINES_H
