//
// Created by Damien Ronssin on 12.08.26.
//

#include "TranscriptionProgress.h"

#include "NeuralNoteTooltips.h"
#include "NnFonts.h"
#include "NnIcons.h"
#include "NnLook.h"
#include "PluginProcessor.h"

namespace
{
const juce::String LOADING_CAPTION = "LOADING MODEL";
const juce::String TRANSCRIBING_CAPTION = "TRANSCRIBING";
constexpr float CAPTION_TRACKING = 0.06f;

constexpr float BAR_CORNER = 2.0f;

// One breath in and out. The caption says the same thing throughout -- what it is for is to say
// that something is still happening between two percentage ticks, which can be seconds apart.
constexpr double PULSE_PERIOD_MS = 1600.0;
constexpr float PULSE_MIN = 0.55f;

int captionWidth(const juce::String& inCaption)
{
    return static_cast<int>(std::ceil(nn::trackedTextWidth(inCaption, nn::fonts::statusBar(), CAPTION_TRACKING)));
}

/** Room for the wider caption, so the group does not change size when the phase does. */
int captionSlotWidth()
{
    return std::max(captionWidth(LOADING_CAPTION), captionWidth(TRANSCRIBING_CAPTION));
}

/** Rounded to a hundredth, so a pulse that has barely moved does not force a repaint. */
float pulseAt(double inMilliseconds)
{
    const auto phase = static_cast<float>(std::fmod(inMilliseconds, PULSE_PERIOD_MS) / PULSE_PERIOD_MS);
    const float eased = 0.5f - 0.5f * std::cos(phase * juce::MathConstants<float>::twoPi);

    return std::round((PULSE_MIN + (1.0f - PULSE_MIN) * eased) * 100.0f) / 100.0f;
}
} // namespace

TranscriptionProgress::TranscriptionProgress(NeuralNoteAudioProcessor& inProcessor)
    : mProcessor(inProcessor)
    , mVBlankAttachment(this, [this]() { _onVBlankCallback(); })
{
    mCancelButton.setIcon(nn::icons::crossStroked, NnFlatButton::IconStyle::stroked, nn::metrics::cancelGlyphSize);
    mCancelButton.setCornerRadius(4.0f);
    mCancelButton.setTooltip(NeuralNoteTooltips::cancel_transcription);
    mCancelButton.setWantsKeyboardFocus(false);

    // Not guarded on mIsCancelling: cancelling is idempotent, and a button that stops responding to
    // the second click is a button the user has to assume is broken.
    mCancelButton.onClick = [this] {
        if (mProcessor.getState() == Processing) {
            mIsCancelling = true;
            mProcessor.getTranscriptionManager()->cancelTranscription();
            repaint();
        }
    };

    addAndMakeVisible(mCancelButton);
}

int TranscriptionProgress::getIdealWidth()
{
    return captionSlotWidth() + nn::metrics::progressBarWidth + nn::metrics::progressPctWidth
           + nn::metrics::cancelHitSize + 3 * nn::metrics::progressGap;
}

void TranscriptionProgress::resized()
{
    mCancelButton.setBounds(getLocalBounds()
                                .removeFromRight(nn::metrics::cancelHitSize)
                                .withSizeKeepingCentre(nn::metrics::cancelHitSize, nn::metrics::cancelHitSize));
}

void TranscriptionProgress::paint(juce::Graphics& g)
{
    auto row = getLocalBounds();
    row.removeFromRight(nn::metrics::cancelHitSize + nn::metrics::progressGap);

    const bool loading = mDisplayedPhase == MuscriptorEngine::Phase::LoadingModel;

    // Dimmed rather than relabelled once cancelling: the run is still going until the engine next
    // checks, and saying otherwise would be a lie for as long as that takes.
    const float alpha = mIsCancelling ? nn::DISABLED_ALPHA : mPulse;

    g.setColour(nn::colours::progressText.withMultipliedAlpha(alpha));
    nn::drawTrackedText(g,
                        loading ? LOADING_CAPTION : TRANSCRIBING_CAPTION,
                        nn::fonts::statusBar(),
                        row.removeFromLeft(captionSlotWidth()).toFloat(),
                        juce::Justification::centredLeft,
                        CAPTION_TRACKING);

    row.removeFromLeft(nn::metrics::progressGap);

    const auto bar = row.removeFromLeft(nn::metrics::progressBarWidth)
                         .withSizeKeepingCentre(nn::metrics::progressBarWidth, nn::metrics::progressBarHeight)
                         .toFloat();

    g.setColour(nn::colours::progressTrack);
    g.fillRoundedRectangle(bar, BAR_CORNER);

    // Nothing to measure yet: the pulsing caption alone says the run is alive.
    if (mDisplayedPercent < 0) {
        return;
    }

    const float filled = bar.getWidth() * static_cast<float>(mDisplayedPercent) / 100.0f;

    if (filled > 0.0f) {
        g.setColour(nn::colours::progressFill.withMultipliedAlpha(mIsCancelling ? nn::DISABLED_ALPHA : 1.0f));
        g.fillRoundedRectangle(bar.withWidth(std::max(filled, 2.0f * BAR_CORNER)), BAR_CORNER);
    }

    row.removeFromLeft(nn::metrics::progressGap);

    g.setColour(nn::colours::progressText.withMultipliedAlpha(mIsCancelling ? nn::DISABLED_ALPHA : 1.0f));
    g.setFont(nn::fonts::statusBar());
    g.drawText(juce::String(mDisplayedPercent) + "%",
               row.removeFromLeft(nn::metrics::progressPctWidth),
               juce::Justification::centredRight);
}

void TranscriptionProgress::_onVBlankCallback()
{
    if (mProcessor.getState() != Processing) {
        // The run is over: drop the latch, the progress and the pulse, so the next one starts fresh.
        mIsCancelling = false;
        mDisplayedPhase = MuscriptorEngine::Phase::LoadingModel;
        mDisplayedPercent = -1;
        mPulse = 1.0f;
        return;
    }

    const auto progress = mProcessor.getTranscriptionManager()->getTranscriptionProgress();
    const int percent = progress.fraction < 0.0f ? -1 : juce::roundToInt(100.0f * progress.fraction);
    const float pulse = pulseAt(static_cast<double>(juce::Time::getMillisecondCounter()));

    if (progress.phase == mDisplayedPhase && percent == mDisplayedPercent && juce::approximatelyEqual(pulse, mPulse)) {
        return;
    }

    mDisplayedPhase = progress.phase;
    mDisplayedPercent = percent;
    mPulse = pulse;
    repaint();
}
