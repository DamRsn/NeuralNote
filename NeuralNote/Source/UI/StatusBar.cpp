//
// Created by Damien Ronssin on 07.08.26.
//

#include "StatusBar.h"

#include "NnFonts.h"
#include "NnIcons.h"
#include "NnLook.h"
#include "PluginProcessor.h"

namespace
{
constexpr int PADDING_SIDE = 14;
constexpr int SEGMENT_GAP = 14;
constexpr int PROGRESS_GAP_TO_ZOOM = 24;

void drawSegments(juce::Graphics& g, const juce::StringArray& inSegments, juce::Rectangle<int> inBounds)
{
    const auto font = nn::fonts::statusBar();
    g.setFont(font);

    const juce::String separator = nn::separatorDot();
    const int separator_width = juce::GlyphArrangement::getStringWidthInt(font, separator);

    auto row = inBounds;

    for (int i = 0; i < inSegments.size(); i++) {
        if (i > 0) {
            row.removeFromLeft(SEGMENT_GAP);
            g.setColour(nn::colours::textSeparator);
            g.drawText(separator, row.removeFromLeft(separator_width), juce::Justification::centredLeft);
            row.removeFromLeft(SEGMENT_GAP);
        }

        const int width = juce::GlyphArrangement::getStringWidthInt(font, inSegments[i]);

        g.setColour(nn::colours::textFainter);
        g.drawText(inSegments[i], row.removeFromLeft(width), juce::Justification::centredLeft);
    }
}
} // namespace

StatusBar::StatusBar(NeuralNoteAudioProcessor& inProcessor)
    : mProcessor(inProcessor)
    , mProgress(inProcessor)
{
    // The bar itself is a label; its two controls are not.
    setInterceptsMouseClicks(false, true);

    addChildComponent(mProgress);

    mZoomSlider.setRange(0.0, 1.0);
    mZoomSlider.setColour(NnFlatSlider::trackColourId, nn::colours::zoomTrack);
    mZoomSlider.setColour(NnFlatSlider::fillColourId, nn::colours::zoomFill);
    mZoomSlider.setColour(NnFlatSlider::thumbColourId, nn::colours::zoomThumb);
    mZoomSlider.setTooltip("Piano roll vertical zoom");
    mZoomSlider.setWantsKeyboardFocus(false);
    mZoomSlider.onValueChange = [this] {
        if (onVerticalZoomChange != nullptr) {
            onVerticalZoomChange(static_cast<float>(mZoomSlider.getValue()));
        }
    };
    addAndMakeVisible(mZoomSlider);

    mProcessor.getInstrumentMixer()->addChangeListener(this);
}

StatusBar::~StatusBar()
{
    mProcessor.getInstrumentMixer()->removeChangeListener(this);
}

void StatusBar::setVerticalZoom(float inNorm)
{
    mZoomSlider.setValue(inNorm, juce::dontSendNotification);
}

void StatusBar::updateEnablements()
{
    mProgress.setVisible(mProcessor.getState() == Processing);
}

void StatusBar::resized()
{
    auto bounds = getLocalBounds().reduced(PADDING_SIDE, 0);

    mZoomSlider.setBounds(bounds.removeFromRight(nn::metrics::zoomTrackWidth));
    bounds.removeFromRight(nn::metrics::zoomGap);
    mZoomIconBounds = bounds.removeFromRight(nn::metrics::zoomIconSize)
                          .withSizeKeepingCentre(nn::metrics::zoomIconSize, nn::metrics::zoomIconSize);

    // Packed against the zoom control rather than centred: the figures on the left grow with the
    // transcription, and a centred group ends up sitting on top of them.
    bounds.removeFromRight(PROGRESS_GAP_TO_ZOOM);
    mProgress.setBounds(bounds.removeFromRight(TranscriptionProgress::getIdealWidth()));
}

void StatusBar::paint(juce::Graphics& g)
{
    g.fillAll(nn::colours::bgPanel);
    nn::drawTopBorder(g, getLocalBounds(), nn::colours::divSoft);

    g.setColour(nn::colours::zoomIcon);
    g.strokePath(
        nn::icons::verticalZoomStroked(mZoomIconBounds.toFloat()),
        juce::PathStrokeType(nn::icons::STROKE_WIDTH, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const InstrumentMixer* mixer = mProcessor.getInstrumentMixer();
    const auto num_instruments = static_cast<int>(mixer->getEntries().size());

    juce::StringArray segments {juce::String(num_instruments) + (num_instruments == 1 ? " instrument" : " instruments"),
                                juce::String(mixer->getTotalNoteCount()) + " notes"};

    // The range and the duration sit with the counts rather than at the far right, which the zoom
    // slider now owns. Each appears only once it means something: an instrument can be selected
    // before it has been transcribed, and its empty range would otherwise read as "C-1 - C-1".
    if (mixer->getTotalNoteCount() > 0) {
        segments.add(nn::midiNoteName(mixer->getLowestPitch()) + " - " + nn::midiNoteName(mixer->getHighestPitch()));
    }

    const double duration = mProcessor.getSourceAudioManager()->getAudioSampleDuration();

    if (duration > 0.0) {
        segments.add(juce::String(duration, 2) + " s");
    }

    drawSegments(g, segments, getLocalBounds().reduced(PADDING_SIDE, 0));
}

void StatusBar::changeListenerCallback(juce::ChangeBroadcaster* inSource)
{
    if (inSource == mProcessor.getInstrumentMixer()) {
        repaint();
    }
}
