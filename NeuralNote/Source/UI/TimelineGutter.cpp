//
// Created by Damien Ronssin on 07.08.26.
//

#include "TimelineGutter.h"

#include "NnFonts.h"
#include "NnLook.h"

namespace
{
constexpr int PADDING_RIGHT = 6;
constexpr float LABEL_HEIGHT = 9.0f;
} // namespace

TimelineGutter::TimelineGutter(int inWaveformHeight)
    : mWaveformHeight(inWaveformHeight)
{
    setInterceptsMouseClicks(false, false);
}

void TimelineGutter::paint(juce::Graphics& g)
{
    g.fillAll(nn::colours::bgGutter);
    nn::drawRightBorder(g, getLocalBounds(), nn::colours::divStrong);

    auto waveform_area = getLocalBounds().removeFromTop(mWaveformHeight);
    nn::drawBottomBorder(g, waveform_area, nn::colours::divSoft);

    auto ruler_area = getLocalBounds().withTrimmedTop(mWaveformHeight);
    nn::drawBottomBorder(g, ruler_area, nn::colours::divSoft);

    waveform_area = waveform_area.withTrimmedRight(PADDING_RIGHT);

    const auto font = nn::fonts::scaleLabel();
    g.setFont(font);

    // Each label is centred on the y its amplitude actually maps to in the waveform, rather than
    // being packed against the top and bottom edges: the scale has to agree with what is drawn
    // next to it, and the font's height is not the amplitude axis.
    const auto label_at = [&](float inAmplitude) {
        const float y = nn::metrics::waveformCentreY - inAmplitude * nn::metrics::waveformAmpHalfSpan;
        return juce::Rectangle<float>(static_cast<float>(waveform_area.getX()),
                                      y - LABEL_HEIGHT / 2.0f,
                                      static_cast<float>(waveform_area.getWidth()),
                                      LABEL_HEIGHT);
    };

    g.setColour(nn::colours::textScale);
    g.drawText("+1.0", label_at(1.0f), juce::Justification::centredRight);
    g.drawText(nn::minusSign() + "1.0", label_at(-1.0f), juce::Justification::centredRight);

    g.setColour(nn::colours::textFaint);
    g.drawText("0", label_at(0.0f), juce::Justification::centredRight);
}
