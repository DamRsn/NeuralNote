//
// Created by Damien Ronssin on 07.08.26.
//

#include "TimeRuler.h"

#include "NnFonts.h"
#include "NnLook.h"
#include "PluginProcessor.h"

namespace
{
// The spacings that read as round numbers of seconds. Anything between them would put labels at
// times nobody thinks in.
constexpr double DIVISIONS[] = {0.1, 0.25, 0.5, 1.0, 2.0, 5.0, 10.0, 15.0, 30.0, 60.0};
constexpr int LABEL_INSET = 6;
} // namespace

TimeRuler::TimeRuler(NeuralNoteAudioProcessor* inProcessor, double inBaseNumPixelsPerSecond)
    : mProcessor(inProcessor)
    , mPlayhead(inProcessor, inBaseNumPixelsPerSecond)
    , mBaseNumPixelsPerSecond(inBaseNumPixelsPerSecond)
{
    mPlayhead.setDrawTriangle(false);
    addAndMakeVisible(mPlayhead);
}

void TimeRuler::resized()
{
    mPlayhead.setBounds(getLocalBounds());
}

void TimeRuler::setZoomLevel(double inZoomLevel)
{
    // Clamped rather than trusted: paint() walks ticks until one lands past the right edge, and at
    // a zoom of zero every tick is at x = 0 and that walk never ends. The caller clamps to its own
    // range already, so this only has to rule out the value that hangs the message thread.
    jassert(inZoomLevel >= MIN_ZOOM_LEVEL);

    mZoomLevel = juce::jmax(MIN_ZOOM_LEVEL, inZoomLevel);
    mPlayhead.setZoomLevel(mZoomLevel);
    repaint();
}

void TimeRuler::paint(juce::Graphics& g)
{
    g.fillAll(nn::colours::bgPanel);
    nn::drawBottomBorder(g, getLocalBounds(), nn::colours::divSoft);

    if (!mProcessor->canPlay()) {
        return;
    }

    const double pixels_per_second = mBaseNumPixelsPerSecond * mZoomLevel;
    const double division = _chooseDivision();
    const auto width = static_cast<double>(getWidth());
    const auto font = nn::fonts::meta();

    g.setFont(font);

    for (int index = 0;; index++) {
        const double time = index * division;
        const double x = std::round(time * pixels_per_second);

        if (x >= width) {
            break;
        }

        g.setColour(nn::colours::divTick);
        g.fillRect(static_cast<int>(x), 0, 1, getHeight());

        const int minutes = static_cast<int>(time) / 60;
        const int seconds = static_cast<int>(time) % 60;

        g.setColour(nn::colours::textFaint);
        g.drawText(juce::String(minutes) + ":" + juce::String(seconds).paddedLeft('0', 2),
                   juce::Rectangle<int>(static_cast<int>(x) + LABEL_INSET, 0, 40, getHeight()),
                   juce::Justification::centredLeft);
    }
}

double TimeRuler::_chooseDivision() const
{
    const double pixels_per_second = mBaseNumPixelsPerSecond * mZoomLevel;

    for (const double division: DIVISIONS) {
        if (division * pixels_per_second >= MIN_LABEL_GAP) {
            return division;
        }
    }

    return DIVISIONS[std::size(DIVISIONS) - 1];
}
