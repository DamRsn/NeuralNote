//
// Created by Damien Ronssin on 07.08.26.
//

#include "NnFonts.h"

#include "BinaryData.h"

namespace nn::fonts
{
namespace
{
    juce::Typeface::Ptr interRegular()
    {
        static const auto face =
            juce::Typeface::createSystemTypefaceFor(BinaryData::InterRegular_ttf, BinaryData::InterRegular_ttfSize);
        return face;
    }

    juce::Typeface::Ptr interMedium()
    {
        static const auto face =
            juce::Typeface::createSystemTypefaceFor(BinaryData::InterMedium_ttf, BinaryData::InterMedium_ttfSize);
        return face;
    }

    juce::Typeface::Ptr interSemiBold()
    {
        static const auto face =
            juce::Typeface::createSystemTypefaceFor(BinaryData::InterSemiBold_ttf, BinaryData::InterSemiBold_ttfSize);
        return face;
    }

    juce::Typeface::Ptr interBold()
    {
        static const auto face =
            juce::Typeface::createSystemTypefaceFor(BinaryData::InterBold_ttf, BinaryData::InterBold_ttfSize);
        return face;
    }

    juce::Typeface::Ptr monoRegular()
    {
        static const auto face = juce::Typeface::createSystemTypefaceFor(BinaryData::JetBrainsMonoNLRegular_ttf,
                                                                         BinaryData::JetBrainsMonoNLRegular_ttfSize);
        return face;
    }

    juce::Typeface::Ptr monoMedium()
    {
        static const auto face = juce::Typeface::createSystemTypefaceFor(BinaryData::JetBrainsMonoNLMedium_ttf,
                                                                         BinaryData::JetBrainsMonoNLMedium_ttfSize);
        return face;
    }

    juce::Typeface::Ptr monoSemiBold()
    {
        static const auto face = juce::Typeface::createSystemTypefaceFor(BinaryData::JetBrainsMonoNLSemiBold_ttf,
                                                                         BinaryData::JetBrainsMonoNLSemiBold_ttfSize);
        return face;
    }

    juce::Typeface::Ptr sansFace(int inWeight)
    {
        if (inWeight >= 700)
            return interBold();
        if (inWeight >= 600)
            return interSemiBold();
        if (inWeight >= 500)
            return interMedium();

        return interRegular();
    }

    juce::Typeface::Ptr monoFace(int inWeight)
    {
        if (inWeight >= 600)
            return monoSemiBold();
        if (inWeight >= 500)
            return monoMedium();

        return monoRegular();
    }
} // namespace

juce::Font sans(float inSizePx, int inWeight)
{
    return juce::Font(juce::FontOptions(sansFace(inWeight))).withPointHeight(inSizePx);
}

juce::Font mono(float inSizePx, int inWeight)
{
    return juce::Font(juce::FontOptions(monoFace(inWeight))).withPointHeight(inSizePx);
}

// A macro would hide what these are; each is one cached Font, and the comment in the header says
// which line of the spec it comes from.
juce::Font wordmark()
{
    static const auto font = sans(15.0f, 600);
    return font;
}

juce::Font wordmarkVersion()
{
    static const auto font = mono(9.0f, 500);
    return font;
}

juce::Font transportTime()
{
    static const auto font = mono(15.0f, 500);
    return font;
}

juce::Font transportTotal()
{
    static const auto font = mono(11.0f, 400);
    return font;
}

juce::Font filename()
{
    static const auto font = sans(12.5f, 500);
    return font;
}

juce::Font instrumentName()
{
    static const auto font = sans(12.0f, 500);
    return font;
}

juce::Font buttonLabel()
{
    static const auto font = sans(11.5f, 500);
    return font;
}

juce::Font menuItem()
{
    static const auto font = sans(11.5f, 400);
    return font;
}

juce::Font menuItemTicked()
{
    // A ticked row is the one thing in a menu the eye should land on, so it carries weight as well
    // as the accent box.
    static const auto font = sans(11.5f, 500);
    return font;
}

juce::Font tempoValue()
{
    static const auto font = mono(11.5f, 400);
    return font;
}

juce::Font sectionHeader()
{
    static const auto font = sans(10.0f, 600);
    return font;
}

juce::Font pillLabel()
{
    static const auto font = sans(9.5f, 500);
    return font;
}

juce::Font statusBar()
{
    static const auto font = mono(9.5f, 400);
    return font;
}

juce::Font meta()
{
    static const auto font = mono(9.0f, 400);
    return font;
}

juce::Font metaStrong()
{
    static const auto font = mono(9.0f, 600);
    return font;
}

juce::Font scaleLabel()
{
    static const auto font = mono(7.5f, 400);
    return font;
}
} // namespace nn::fonts
