//
// Created by Damien Ronssin on 11.03.23.
//

#include "VisualizationPanel.h"

#include <NeuralNoteTooltips.h>

VisualizationPanel::VisualizationPanel(NeuralNoteAudioProcessor* processor)
    : mProcessor(processor)
    , mCombinedAudioMidiRegion(processor, mKeyboard)
    , mMidiFileDrag(processor)
{
    mAudioMidiViewport.setViewedComponent(&mCombinedAudioMidiRegion);
    addAndMakeVisible(mAudioMidiViewport);
    mCombinedAudioMidiRegion.setViewportPtr(&mAudioMidiViewport);

    addAndMakeVisible(mKeyboard);

    mAudioMidiViewport.setScrollBarsShown(false, true, false, false);
    addChildComponent(mMidiFileDrag);

    auto tempo_str_validator = [](const String& tempo_str) {
        if (tempo_str.isEmpty()) {
            return false;
        }

        float tempo = tempo_str.getFloatValue();
        return tempo >= 20.0f && tempo <= 999.0f;
    };

    auto tempo_str_corrector = [](const String& tempo_str) {
        return tempo_str.isEmpty() ? String("120") : String(jlimit(20.0f, 999.0f, tempo_str.getFloatValue()));
    };

    mFileTempo = std::make_unique<NumericTextEditor<double>>(
        mProcessor, NnId::ExportTempoId, 6, 120.0, Justification::centred, tempo_str_validator, tempo_str_corrector);
    mFileTempo->setTooltip(NeuralNoteTooltips::export_tempo);
    addChildComponent(*mFileTempo);

    mAudioGainSlider.setSliderStyle(Slider::SliderStyle::LinearHorizontal);
    mAudioGainSlider.setTextBoxStyle(Slider::TextEntryBoxPosition::TextBoxLeft, true, 40, 20);
    mAudioGainSlider.setTextValueSuffix(" dB");
    mAudioGainSlider.setColour(Slider::ColourIds::textBoxTextColourId, BLACK);
    mAudioGainSlider.setColour(Slider::ColourIds::textBoxOutlineColourId, Colours::transparentWhite);
    // To also receive mouseExit callback from this slider
    mAudioGainSlider.addMouseListener(this, true);
    mAudioGainSlider.setTooltip(NeuralNoteTooltips::source_audio_level);
    mAudioGainSliderAttachment = std::make_unique<SliderParameterAttachment>(
        *mProcessor->getParams()[ParameterHelpers::AudioPlayerGainId], mAudioGainSlider);

    addChildComponent(mAudioGainSlider);

    mMidiGainSlider.setSliderStyle(Slider::SliderStyle::LinearHorizontal);
    mMidiGainSlider.setTextBoxStyle(Slider::TextEntryBoxPosition::TextBoxLeft, true, 40, 20);
    mMidiGainSlider.setTextValueSuffix(" dB");
    mMidiGainSlider.setColour(Slider::ColourIds::textBoxTextColourId, BLACK);
    mMidiGainSlider.setColour(Slider::ColourIds::textBoxOutlineColourId, Colours::transparentWhite);
    // To also receive mouseExit callback from this slider
    mMidiGainSlider.addMouseListener(this, true);
    mMidiGainSlider.setTooltip(NeuralNoteTooltips::internal_synth_level);

    mMidiGainSliderAttachment = std::make_unique<SliderParameterAttachment>(
        *mProcessor->getParams()[ParameterHelpers::MidiPlayerGainId], mMidiGainSlider);

    addChildComponent(mMidiGainSlider);

    // Add this as mouse listener of audio region and pianoroll to control visibility of gain sliders
    mCombinedAudioMidiRegion.getAudioRegion()->addMouseListener(this, true);
    mCombinedAudioMidiRegion.getPianoRoll()->addMouseListener(this, true);
}

void VisualizationPanel::resized()
{
    mKeyboard.setBounds(
        0, mCombinedAudioMidiRegion.mPianoRollY, KEYBOARD_WIDTH, getHeight() - mCombinedAudioMidiRegion.mPianoRollY);

    mAudioMidiViewport.setBounds(KEYBOARD_WIDTH, 0, getWidth() - KEYBOARD_WIDTH, getHeight());

    mCombinedAudioMidiRegion.setBaseWidth(getWidth() - KEYBOARD_WIDTH);
    mCombinedAudioMidiRegion.setBounds(KEYBOARD_WIDTH, 0, getWidth() - KEYBOARD_WIDTH, getHeight());
    mCombinedAudioMidiRegion.changeListenerCallback(mProcessor->getSourceAudioManager()->getAudioThumbnail());

    mMidiFileDrag.setBounds(0, mCombinedAudioMidiRegion.mPianoRollY - 13, getWidth(), 13);
    mFileTempo->setBounds(6, 55, 40, 14);

    mAudioGainSlider.setBounds(getWidth() - 205, 3, 200, 20);
    mMidiGainSlider.setBounds(getWidth() - 205, mCombinedAudioMidiRegion.mPianoRollY + 3, 200, 20);

    mAudioRegionBounds = {KEYBOARD_WIDTH, 0, getWidth() - KEYBOARD_WIDTH, mCombinedAudioMidiRegion.mAudioRegionHeight};
    mPianoRollBounds = {
        KEYBOARD_WIDTH,
        mCombinedAudioMidiRegion.mAudioRegionHeight + mCombinedAudioMidiRegion.mHeightBetweenAudioMidi,
        getWidth() - KEYBOARD_WIDTH,
        getHeight() - (mCombinedAudioMidiRegion.mAudioRegionHeight + mCombinedAudioMidiRegion.mHeightBetweenAudioMidi)};
}

void VisualizationPanel::paint(Graphics& g)
{
    if (mMidiFileDrag.isVisible()) {
        g.setColour(WHITE_TRANSPARENT);
        g.fillRoundedRectangle(
            Rectangle<int>(0, 0, KEYBOARD_WIDTH, mCombinedAudioMidiRegion.mAudioRegionHeight).toFloat(), 4);

        g.setColour(BLACK);
        g.setFont(UIDefines::LABEL_FONT());
        g.drawFittedText("MIDI\nFILE\nTEMPO", Rectangle<int>(0, 0, KEYBOARD_WIDTH, 55), Justification::centred, 3);
    }
}

void VisualizationPanel::clear()
{
    mCombinedAudioMidiRegion.setSize(getWidth() - KEYBOARD_WIDTH, getHeight());
    mMidiFileDrag.setVisible(false);
    mFileTempo->setVisible(false);
}

void VisualizationPanel::repaintPianoRoll()
{
    mCombinedAudioMidiRegion.repaintPianoRoll();
}

void VisualizationPanel::setMidiFileDragComponentVisible()
{
    mMidiFileDrag.setVisible(true);
    mFileTempo->setVisible(true);
}

void VisualizationPanel::mouseEnter(const MouseEvent& event)
{
    Component::mouseEnter(event);

    if (mProcessor->getState() == PopulatedAudioAndMidiRegions) {
        if (event.originalComponent == mCombinedAudioMidiRegion.getAudioRegion()) {
            mAudioGainSlider.setVisible(true);
        } else if (event.originalComponent == mCombinedAudioMidiRegion.getPianoRoll()) {
            mMidiGainSlider.setVisible(true);
        }
    }
}

void VisualizationPanel::mouseExit(const MouseEvent& event)
{
    Component::mouseExit(event);

    if (mAudioGainSlider.isVisible()) {
        if (!mAudioRegionBounds.contains(getMouseXYRelative()))
            mAudioGainSlider.setVisible(false);
    }

    if (mMidiGainSlider.isVisible()) {
        if (!mPianoRollBounds.contains(getMouseXYRelative()))
            mMidiGainSlider.setVisible(false);
    }
}

Viewport& VisualizationPanel::getAudioMidiViewport()
{
    return mAudioMidiViewport;
}

CombinedAudioMidiRegion& VisualizationPanel::getCombinedAudioMidiRegion()
{
    return mCombinedAudioMidiRegion;
}
