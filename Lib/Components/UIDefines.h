//
// Created by Damien Ronssin on 12.03.23.
//

#ifndef AUDIO2MIDIPLUGIN_UIDEFINES_H
#define AUDIO2MIDIPLUGIN_UIDEFINES_H

#include <JuceHeader.h>
#include "BinaryData.h"

const juce::Typeface::Ptr MONTSERRAT_BOLD = juce::Typeface::createSystemTypefaceFor(
    BinaryData::MontserratBold_ttf, BinaryData::MontserratBold_ttfSize);

const juce::Typeface::Ptr MONTSERRAT_SEMIBOLD = juce::Typeface::createSystemTypefaceFor(
    BinaryData::MontserratSemiBold_ttf, BinaryData::MontserratSemiBold_ttfSize);

const juce::Typeface::Ptr MONTSERRAT_REGULAR = juce::Typeface::createSystemTypefaceFor(
    BinaryData::MontserratRegular_ttf, BinaryData::MontserratRegular_ttfSize);

const Font TITLE_FONT = Font(MONTSERRAT_BOLD).withPointHeight(14.0f);
const Font LABEL_FONT = Font(MONTSERRAT_SEMIBOLD).withPointHeight(10.0f);
const Font DROPDOWN_FONT = Font(MONTSERRAT_REGULAR).withPointHeight(10.0f);
const Font BUTTON_FONT = Font(MONTSERRAT_BOLD).withPointHeight(12.0f);

#endif //AUDIO2MIDIPLUGIN_UIDEFINES_H
