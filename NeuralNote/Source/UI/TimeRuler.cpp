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
constexpr int LABEL_INSET = 8;
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
    // Lift the ruler slightly from the waveform area, with a clearer lower boundary.
    g.fillAll(nn::colours::bgPanel);
    g.setColour(juce::Colours::white.withAlpha(0.56f));
    g.fillRect(getLocalBounds().toFloat());
    nn::drawBottomBorder(g, getLocalBounds(), nn::colours::divStrong);

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

        const bool majorTick = (std::fmod(time, 5.0) < 0.001 || std::fmod(time, 5.0) > 4.999);
        g.setColour(majorTick ? nn::colours::divStrong : nn::colours::divTick);
        g.fillRect(static_cast<int>(x), majorTick ? 0 : getHeight() / 2, 1, majorTick ? getHeight() : getHeight() / 2);

        const int minutes = static_cast<int>(time) / 60;
        const int seconds = static_cast<int>(time) % 60;
        const juce::String label = juce::String(minutes) + ":" + juce::String(seconds).paddedLeft('0', 2);
        const juce::Rectangle<float> labelBounds(static_cast<float>(x) + LABEL_INSET,
                                                   (static_cast<float>(getHeight()) - 18.0f) / 2.0f,
                                                   44.0f,
                                                   18.0f);

        if (majorTick) {
            // Five-second anchors get a quiet capsule, making the timeline easier to scan.
            g.setColour(nn::colours::bgControlAlt.withAlpha(0.78f));
            g.fillRoundedRectangle(labelBounds, 5.0f);
            g.setColour(nn::colours::divSoft);
            g.drawRoundedRectangle(labelBounds.reduced(0.5f), 5.0f, 0.8f);
            g.setColour(nn::colours::textDim);
        } else {
            g.setColour(nn::colours::textFaint);
        }

        g.drawText(label, labelBounds.reduced(4.0f, 0.0f), juce::Justification::centredLeft);
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
