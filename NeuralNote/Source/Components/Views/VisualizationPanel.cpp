//
// Created by Damien Ronssin on 11.03.23.
//

#include "VisualizationPanel.h"

#include "InstrumentSelection.h"
#include "NeuralNoteTooltips.h"
#include "NNFileUtils.h"
#include "NnFonts.h"
#include "NnIcons.h"
#include "NnLook.h"

VisualizationPanel::VisualizationPanel(NeuralNoteAudioProcessor* processor)
    : mProcessor(processor)
    , mToolbar(*processor)
    , mGutter(nn::metrics::waveformHeight)
    , mCombinedAudioMidiRegion(processor, mKeyboard)
    , mStatusBar(*processor)
    , mModelDownloadPanel(*processor)
{
    addAndMakeVisible(mToolbar);
    addAndMakeVisible(mGutter);
    addAndMakeVisible(mKeyboard);

    mAudioMidiViewport.setViewedComponent(&mCombinedAudioMidiRegion, false);
    mAudioMidiViewport.setScrollBarsShown(false, true, false, false);

    // Recoloured through the scrollbar's own colour ids rather than a LookAndFeel; the default is
    // a saturated blue that reads as an accent this palette does not have.
    auto& scrollbar = mAudioMidiViewport.getHorizontalScrollBar();
    scrollbar.setColour(ScrollBar::backgroundColourId, Colours::transparentBlack);
    scrollbar.setColour(ScrollBar::thumbColourId, nn::colours::faderTrack);
    scrollbar.setColour(ScrollBar::trackColourId, Colours::transparentBlack);

    addAndMakeVisible(mAudioMidiViewport);
    mCombinedAudioMidiRegion.setViewportPtr(&mAudioMidiViewport);

    addAndMakeVisible(mStatusBar);

    // Loading a file no longer starts a transcription -- this is what does, once the user has had
    // the chance to pick instruments. Only on screen while the roll is empty and idle, and its
    // label is set in _layOutTranscribeButton, which knows what the selection is.
    mTranscribeButton.setIcon(nn::icons::transcribeStroked, NnFlatButton::IconStyle::stroked, 15.0f);
    mTranscribeButton.setPadding(nn::metrics::ctaPadX, nn::metrics::ctaPadX, nn::metrics::ctaGap);
    mTranscribeButton.setCornerRadius(nn::metrics::controlCorner);
    mTranscribeButton.setColour(NnFlatButton::backgroundColourId, nn::colours::ctaFill());
    mTranscribeButton.setColour(NnFlatButton::outlineColourId, nn::colours::ctaBorder);
    mTranscribeButton.setColour(NnFlatButton::iconColourId, nn::colours::ctaText);
    mTranscribeButton.setColour(NnFlatButton::textColourId, nn::colours::ctaText);
    mTranscribeButton.setTooltip(NeuralNoteTooltips::transcribe);
    mTranscribeButton.onClick = [this] { mProcessor->getTranscriptionManager()->launchTranscribeJob(); };
    addChildComponent(mTranscribeButton);

    mModelDownloadPanel.onInstalledModelsChanged = [this] { _layOutTranscribeButton(); };
    mModelDownloadPanel.onCloseRequested = [this] { setModelPanelOpen(false); };
    addChildComponent(mModelDownloadPanel);

    mStatusBar.onVerticalZoomChange = [this](float inNorm) {
        mProcessor->getValueTree().setPropertyExcludingListener(this, NnId::VerticalZoomId, inNorm, nullptr);
        _applyVerticalZoom();
    };

    // The mixer reports the transcription's pitch range, and re-reports it once per decoded chunk.
    mProcessor->getInstrumentMixer()->addChangeListener(this);
    mProcessor->addListenerToStateValueTree(this);
}

VisualizationPanel::~VisualizationPanel()
{
    mProcessor->removeListenerFromStateValueTree(this);
    mProcessor->getInstrumentMixer()->removeChangeListener(this);
}

void VisualizationPanel::valueTreePropertyChanged(juce::ValueTree& inTree, const juce::Identifier& inProperty)
{
    juce::ignoreUnused(inTree);

    // Reset Zoom writes it back to automatic, and a restored session brings its own value.
    if (inProperty == NnId::VerticalZoomId) {
        _applyVerticalZoom();
    }
}

void VisualizationPanel::_applyVerticalZoom()
{
    if (mKeyboard.getHeight() <= 0) {
        return;
    }

    const double stored = mProcessor->getValueTree().getProperty(NnId::VerticalZoomId, -1.0);
    float norm = static_cast<float>(stored);

    if (stored < 0.0) {
        // Fit to the octaves the transcription occupies, not to the notes themselves: those octaves
        // are what the roll ends up drawing, so fitting to anything narrower crops it. Passing a
        // key width of zero is what asks computeDisplayRange for them without the fill rule.
        const InstrumentMixer* mixer = mProcessor->getInstrumentMixer();
        const PianoRollRange::DisplayRange content = PianoRollRange::computeDisplayRange(
            mixer->getLowestPitch(), mixer->getHighestPitch(), mixer->getTotalNoteCount() > 0, 0.0f, 0.0f);

        norm = nn::zoom::fitToContent(mKeyboard.getHeight(), content.highNote - content.lowNote + 1);
    }

    mKeyboard.setWhiteKeyHeight(nn::zoom::keyHeight(norm));
    mStatusBar.setVerticalZoom(norm);

    // The range has to be re-derived against the new key height: zoomed out it widens to fill the
    // column, zoomed in it stays on the content and the keyboard scrolls within it.
    _updateNoteRange(mProcessor->getState() != Processing);
}

void VisualizationPanel::changeListenerCallback(juce::ChangeBroadcaster* inSource)
{
    if (inSource == mProcessor->getInstrumentMixer()) {
        _updateNoteRange(false);
    }
}

void VisualizationPanel::_updateNoteRange(bool inMayShrink)
{
    const InstrumentMixer* mixer = mProcessor->getInstrumentMixer();
    const bool has_notes = mixer->getTotalNoteCount() > 0;

    PianoRollRange::DisplayRange range = PianoRollRange::computeDisplayRange(mixer->getLowestPitch(),
                                                                             mixer->getHighestPitch(),
                                                                             has_notes,
                                                                             static_cast<float>(mKeyboard.getHeight()),
                                                                             mKeyboard.getKeyWidth());

    if (!inMayShrink) {
        range = PianoRollRange::unionOf(range, mKeyboard.getDisplayedRange());
    }

    mKeyboard.setDimmed(!has_notes);

    if (range != mKeyboard.getDisplayedRange()) {
        mKeyboard.setDisplayedRange(range);
        mCombinedAudioMidiRegion.repaintPianoRoll();
    }
}

void VisualizationPanel::resized()
{
    auto bounds = getLocalBounds();

    mToolbar.setBounds(bounds.removeFromTop(nn::metrics::toolbarHeight));
    mStatusBar.setBounds(bounds.removeFromBottom(nn::metrics::statusBarHeight));

    // The gutter and the keyboard sit outside the viewport, at the same width the waveform, ruler
    // and piano roll each reserve on their left. That is what aligns the three time axes.
    auto gutter_column = bounds.removeFromLeft(nn::metrics::timelineGutter);
    mGutter.setBounds(gutter_column.removeFromTop(nn::metrics::waveformHeight + nn::metrics::rulerHeight));
    mKeyboard.setBounds(gutter_column);

    mAudioMidiViewport.setBounds(bounds);

    // Only the size: the viewport owns where its viewed component sits, and moves it on scroll.
    mCombinedAudioMidiRegion.setBaseWidth(bounds.getWidth());
    mCombinedAudioMidiRegion.setSize(bounds.getWidth(), bounds.getHeight());
    mCombinedAudioMidiRegion.refreshForAudioLength();

    _layOutTranscribeButton();

    // The keyboard's height is what an automatic zoom is fitted against, and what decides how many
    // octaves have to be on screen.
    _applyVerticalZoom();
}

void VisualizationPanel::_layOutTranscribeButton()
{
    const State state = mProcessor->getState();
    const bool roll_is_idle = state == AudioLoaded || state == EmptyAudioAndMidiRegions;
    const bool has_model = NNFileUtils::isAnyModelInstalled();

    // Centred on the piano roll, which is the viewport minus the waveform and ruler above it.
    const auto roll =
        mAudioMidiViewport.getBounds().withTrimmedTop(nn::metrics::waveformHeight + nn::metrics::rulerHeight);

    // With no checkpoint, the panel is the only thing to do next on an idle roll, so it cannot be
    // closed there. Anywhere else it is up only when asked for, over whatever is showing.
    const bool panel_is_required = roll_is_idle && !has_model;
    mModelDownloadPanel.setCloseButtonVisible(!panel_is_required);
    mModelDownloadPanel.setVisible(mModelPanelOpen || panel_is_required);
    mModelDownloadPanel.setBounds(
        roll.withSizeKeepingCentre(std::min(ModelDownloadPanel::getIdealWidth(), roll.getWidth()),
                                   std::min(ModelDownloadPanel::getIdealHeight(), roll.getHeight())));

    // Shown before there is any audio too, disabled: the empty roll is where the user looks for
    // what happens next, and an absent button says less than an unavailable one.
    mTranscribeButton.setVisible(roll_is_idle && has_model);
    mTranscribeButton.setEnabled(state == AudioLoaded);

    if (!mTranscribeButton.isVisible()) {
        return;
    }

    // Says what it is about to do, because the picker sits across the window in the sidebar and
    // the difference between a constrained run and a free one is minutes of work either way. With
    // no audio there is nothing to be specific about, so it names the action only.
    const auto instrument_count =
        state == AudioLoaded ? static_cast<int>(InstrumentSelection::get(mProcessor->getValueTree()).size()) : 0;

    mTranscribeButton.setLabel(instrument_count == 0 ? juce::String("Transcribe")
                               : instrument_count == 1
                                   ? juce::String("Transcribe 1 instrument")
                                   : "Transcribe " + juce::String(instrument_count) + " instruments",
                               nn::fonts::buttonLabel());

    mTranscribeButton.setBounds(roll.withSizeKeepingCentre(mTranscribeButton.getIdealWidth(), nn::metrics::ctaHeight));
}

void VisualizationPanel::clear()
{
    // Resizes back to the base width from the (now zero) number of samples available, rather than
    // waiting on the thumbnail's asynchronous change message.
    mCombinedAudioMidiRegion.refreshForAudioLength();
    mAudioMidiViewport.setViewPosition(0, 0);

    updateEnablements();
    repaint();
}

void VisualizationPanel::updateEnablements()
{
    mToolbar.updateEnablements();
    mCombinedAudioMidiRegion.updateEnablements();
    mStatusBar.updateEnablements();
    _layOutTranscribeButton();

    const State state = mProcessor->getState();

    if (state != mPrevStateForRange) {
        // Every state change settles the view on what is actually there -- which is what closes the
        // range in once a run finishes, and what returns it to the default once one is cleared. The
        // exception is a run in flight, where each chunk may widen it and nothing may narrow it,
        // so the view does not jump under the user between chunks.
        mPrevStateForRange = state;
        _applyVerticalZoom();
    }
}

void VisualizationPanel::setModelPanelOpen(bool inOpen)
{
    mModelPanelOpen = inOpen;
    _layOutTranscribeButton();
}

bool VisualizationPanel::isModelPanelVisible() const
{
    return mModelDownloadPanel.isVisible();
}

void VisualizationPanel::repaintPianoRoll()
{
    mCombinedAudioMidiRegion.repaintPianoRoll();
}

Viewport& VisualizationPanel::getAudioMidiViewport()
{
    return mAudioMidiViewport;
}

CombinedAudioMidiRegion& VisualizationPanel::getCombinedAudioMidiRegion()
{
    return mCombinedAudioMidiRegion;
}
