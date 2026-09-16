//
// Created by Damien Ronssin on 07.08.26.
//

#include "TimeDisplay.h"

#include "NnFonts.h"
#include "NnLook.h"
#include "PluginProcessor.h"

namespace
{
constexpr int SIDE_PADDING = 14;
constexpr int GAP = 8;
} // namespace

TimeDisplay::TimeDisplay(NeuralNoteAudioProcessor* inProcessor)
    : mProcessor(inProcessor)
    , mVBlankAttachment(this, [this]() { _onVBlankCallback(); })
{
    setInterceptsMouseClicks(false, false);
}

int TimeDisplay::getIdealWidth()
{
    const int position_width = juce::GlyphArrangement::getStringWidthInt(nn::fonts::transportTime(), "00:00.00");
    const int total_width = juce::GlyphArrangement::getStringWidthInt(nn::fonts::transportTotal(), "/ 00:00.00");

    return 2 * SIDE_PADDING + position_width + GAP + total_width;
}

void TimeDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // The 1 px rules that separate the readout from the transport and from the empty stretch after
    // it. Drawn here rather than by the top bar so they cannot drift from the text they bracket.
    g.setColour(nn::colours::divStrong);
    g.fillRect(bounds.getX(), bounds.getY(), 1, bounds.getHeight());
    g.fillRect(bounds.getRight() - 1, bounds.getY(), 1, bounds.getHeight());

    bounds.reduce(SIDE_PADDING, 0);

    const auto position_font = nn::fonts::transportTime();
    const int position_width = juce::GlyphArrangement::getStringWidthInt(position_font, mPosition);

    // A zero that cannot move is not a reading. Held back until there is audio, alongside the
    // total's placeholder.
    g.setColour(mProcessor->canPlay() ? nn::colours::textBright : nn::colours::textScale);
    g.setFont(position_font);
    g.drawText(mPosition, bounds.removeFromLeft(position_width), juce::Justification::centredLeft);

    bounds.removeFromLeft(GAP);

    g.setColour(nn::colours::textFaint);
    g.setFont(nn::fonts::transportTotal());
    g.drawText("/ " + mTotal, bounds, juce::Justification::centredLeft);
}

void TimeDisplay::_onVBlankCallback()
{
    const double duration = mProcessor->getSourceAudioManager()->getAudioSampleDuration();

    const juce::String position = _format(mProcessor->getPlayer()->getPlayheadPositionSeconds());
    const juce::String total = duration > 0.0 ? _format(duration) : juce::String("--:--.--");

    if (position != mPosition || total != mTotal) {
        mPosition = position;
        mTotal = total;
        repaint();
    }
}

juce::String TimeDisplay::_format(double inSeconds)
{
    const int hundredths = juce::jmax(0, juce::roundToInt(inSeconds * 100.0));

    return juce::String(hundredths / 6000).paddedLeft('0', 2) + ":"
           + juce::String((hundredths / 100) % 60).paddedLeft('0', 2) + "."
           + juce::String(hundredths % 100).paddedLeft('0', 2);
}
