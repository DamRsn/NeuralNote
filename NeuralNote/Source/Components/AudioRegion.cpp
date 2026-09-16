//
// Created by Damien Ronssin on 09.03.23.
//

#include "AudioRegion.h"

#include "CombinedAudioMidiRegion.h"
#include "NeuralNoteTooltips.h"
#include "NnFonts.h"
#include "NnIcons.h"
#include "NnLook.h"
#include "WaveformBars.h"

namespace
{
constexpr int CORNER_LABEL_INSET = 10;

/** So a near-silent passage still reads as audio rather than as an empty region. */
constexpr float MIN_BAR_HEIGHT = 1.0f;

constexpr float DASH_LENGTHS[] = {4.0f, 4.0f};
constexpr int DROP_HINT_HEIGHT = 12;
constexpr float DROP_HINT_TRACKING = 0.06f;
} // namespace

AudioRegion::AudioRegion(NeuralNoteAudioProcessor* processor, double inBaseNumPixelsPerSecond)
    : mProcessor(processor)
    , mPlayhead(processor, inBaseNumPixelsPerSecond)
    , mVBlankAttachment(this, [this]() { _onVBlankCallback(); })
    , mBaseNumPixelsPerSecond(inBaseNumPixelsPerSecond)
{
    addAndMakeVisible(mPlayhead);

    mLoadButton.setIcon(nn::icons::folderStroked, NnFlatButton::IconStyle::stroked, 14.0f);
    mLoadButton.setLabel("Load audio file", nn::fonts::buttonLabel());
    mLoadButton.setPadding(nn::metrics::loadPadX, nn::metrics::loadPadX, nn::metrics::loadGap);
    mLoadButton.setCornerRadius(nn::metrics::controlCorner);
    mLoadButton.setColour(NnFlatButton::backgroundColourId, nn::colours::accentFillButton());
    mLoadButton.setColour(NnFlatButton::outlineColourId, nn::colours::ctaBorder);
    mLoadButton.setColour(NnFlatButton::iconColourId, nn::colours::ctaText);
    mLoadButton.setColour(NnFlatButton::textColourId, nn::colours::ctaText);
    mLoadButton.setTooltip(NeuralNoteTooltips::load_audio);
    mLoadButton.onClick = [this] { _openFileChooser(); };
    addChildComponent(mLoadButton);

    updateEnablements();
}

void AudioRegion::resized()
{
    mPlayhead.setSize(getWidth(), getHeight());

    // The button and the hint below it are centred as a column, so the button's own centre sits
    // slightly above the region's.
    const int column_height = nn::metrics::loadHeight + nn::metrics::dropHintGap + DROP_HINT_HEIGHT;

    mLoadButton.setBounds(getLocalBounds()
                              .withSizeKeepingCentre(mLoadButton.getIdealWidth(), column_height)
                              .removeFromTop(nn::metrics::loadHeight));
}

void AudioRegion::updateEnablements()
{
    mLoadButton.setVisible(mProcessor->getState() == EmptyAudioAndMidiRegions);
}

void AudioRegion::paint(Graphics& g)
{
    g.fillAll(nn::colours::bgPanel);
    nn::drawBottomBorder(g, getLocalBounds(), nn::colours::divSoft);

    const auto num_samples_available = mProcessor->getSourceAudioManager()->getNumSamplesDownAcquired();

    if (num_samples_available <= 0) {
        _paintDropZone(g);
        return;
    }

    g.setColour(nn::colours::waveCentreLine());
    g.fillRect(0, getHeight() / 2, getWidth(), 1);

    _paintWaveform(g);

    const auto playhead_x = static_cast<int>(std::round(mPlayhead.getPlayheadX()));

    if (playhead_x > 0) {
        g.setColour(nn::colours::accentWashWave());
        g.fillRect(0, 0, playhead_x, getHeight());

        g.setColour(nn::colours::accentWashEdge());
        g.fillRect(playhead_x - 1, 0, 1, getHeight());
    }

    g.setColour(nn::colours::textScale);
    nn::drawTrackedText(g,
                        "MIX WAVEFORM",
                        nn::fonts::meta(),
                        juce::Rectangle<float>(CORNER_LABEL_INSET, CORNER_LABEL_INSET, 200.0f, 12.0f),
                        juce::Justification::topLeft,
                        0.1f);
}

void AudioRegion::_paintWaveform(Graphics& g) const
{
    const double pixels_per_second = mBaseNumPixelsPerSecond * mZoomLevel;

    const WaveformPeaks::Reader reader(mProcessor->getSourceAudioManager()->getWaveformPeaks());
    const int64_t num_samples = reader.getNumSamples();

    // This component is as wide as the whole timeline -- up to a few hundred thousand pixels -- so
    // what is drawn has to follow the exposed sliver, not the component.
    const auto clip = g.getClipBounds();

    const auto bars = WaveformBars::visibleBars(static_cast<float>(clip.getX()),
                                                static_cast<float>(clip.getRight()),
                                                nn::metrics::waveformBarPitch,
                                                num_samples,
                                                pixels_per_second,
                                                TRANSCRIPTION_SAMPLE_RATE);

    if (bars.isEmpty()) {
        return;
    }

    const auto amplitude_to_y = [](float inAmplitude) {
        return nn::metrics::waveformCentreY
               - juce::jlimit(-1.0f, 1.0f, inAmplitude) * nn::metrics::waveformAmpHalfSpan;
    };

    juce::RectangleList<float> waveform;
    waveform.ensureStorageAllocated(static_cast<int>(bars.count()));

    auto bar_start = WaveformBars::barStartSample(
        bars.firstBar, nn::metrics::waveformBarPitch, pixels_per_second, TRANSCRIPTION_SAMPLE_RATE);

    for (int64_t bar = bars.firstBar; bar <= bars.lastBar; ++bar) {
        const auto next_start = WaveformBars::barStartSample(
            bar + 1, nn::metrics::waveformBarPitch, pixels_per_second, TRANSCRIPTION_SAMPLE_RATE);
        const auto peaks = reader.query(bar_start, next_start);
        bar_start = next_start;

        if (peaks.isEmpty()) {
            continue;
        }

        const float high = mSymmetricBars ? std::max(std::abs(peaks.min), std::abs(peaks.max)) : peaks.max;
        const float low = mSymmetricBars ? -high : peaks.min;

        const float top = amplitude_to_y(high);
        const float bottom = amplitude_to_y(low);
        const float height = std::max(bottom - top, MIN_BAR_HEIGHT);

        // Integer x keeps the bar edges crisp; the y stays fractional so the envelope reads as a
        // curve rather than a staircase.
        waveform.addWithoutMerging({static_cast<float>(bar) * nn::metrics::waveformBarPitch,
                                    (top + bottom - height) / 2.0f,
                                    nn::metrics::waveformBarWidth,
                                    height});
    }

    g.setColour(nn::colours::wavePlayed);
    g.fillRectList(waveform);
}

void AudioRegion::_paintDropZone(Graphics& g) const
{
    const auto zone = getLocalBounds().reduced(nn::metrics::dropZoneInset).toFloat();

    g.setColour(mIsFileOver ? nn::colours::ctaFill() : nn::colours::dropZoneFill());
    g.fillRoundedRectangle(zone, nn::metrics::dropZoneCorner);

    // Dashed rather than solid: a solid outline this size reads as a panel that is part of the
    // layout, and this one goes away as soon as anything is loaded.
    Path outline;
    outline.addRoundedRectangle(zone.reduced(0.5f), nn::metrics::dropZoneCorner);

    Path dashed;
    PathStrokeType(1.0f).createDashedStroke(dashed, outline, DASH_LENGTHS, numElementsInArray(DASH_LENGTHS));

    g.setColour(mIsFileOver ? nn::colours::ctaBorder : nn::colours::dropZoneBorder);
    g.fillPath(dashed);

    auto hint =
        getLocalBounds()
            .withSizeKeepingCentre(getWidth(), nn::metrics::loadHeight + nn::metrics::dropHintGap + DROP_HINT_HEIGHT)
            .removeFromBottom(DROP_HINT_HEIGHT);

    g.setColour(nn::colours::textScale);
    nn::drawTrackedText(
        g, "OR DROP A FILE HERE", nn::fonts::meta(), hint.toFloat(), juce::Justification::centred, DROP_HINT_TRACKING);
}

void AudioRegion::_openFileChooser()
{
    // Built from the formats the loader actually accepts, rather than a second hard-coded list that
    // can drift from it.
    StringArray patterns;

    for (const String& extension: AudioUtils::getSupportedAudioFileExtensions()) {
        patterns.add("*" + extension);
    }

    mFileChooser = std::make_shared<juce::FileChooser>(
        "Select Audio File", juce::File {}, patterns.joinIntoString(";"), true, false, this);

    mFileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this](const juce::FileChooser& fc) {
                                  if (fc.getResults().isEmpty())
                                      return;

                                  auto* parent = dynamic_cast<CombinedAudioMidiRegion*>(getParentComponent());

                                  if (parent) {
                                      parent->filesDropped(StringArray(fc.getResult().getFullPathName()), 1, 1);
                                  }
                              });
}

void AudioRegion::setIsFileOver(bool inIsFileOver)
{
    mIsFileOver = inIsFileOver;
}

void AudioRegion::mouseDown(const juce::MouseEvent& e)
{
    if (mProcessor->canPlay()) {
        mPlayhead.setPlayheadTime(_pixelToTime(static_cast<float>(e.x)));
    }
}

void AudioRegion::setZoomLevel(double inZoomLevel)
{
    mZoomLevel = inZoomLevel;
    mPlayhead.setZoomLevel(inZoomLevel);
    repaint();
}

void AudioRegion::setSymmetricBars(bool inSymmetric)
{
    if (mSymmetricBars != inSymmetric) {
        mSymmetricBars = inSymmetric;
        repaint();
    }
}

void AudioRegion::_onVBlankCallback()
{
    const auto playhead_x = static_cast<int>(std::round(mPlayhead.getPlayheadX()));

    if (playhead_x == mLastPlayheadX) {
        return;
    }

    const int from = std::min(playhead_x, mLastPlayheadX);
    const int to = std::max(playhead_x, mLastPlayheadX);
    mLastPlayheadX = playhead_x;

    repaint(from - 2, 0, to - from + 4, getHeight());
}

float AudioRegion::_pixelToTime(float inPixel) const
{
    return inPixel / static_cast<float>(mBaseNumPixelsPerSecond * mZoomLevel);
}
