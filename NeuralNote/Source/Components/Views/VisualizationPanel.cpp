//
// Created by Damien Ronssin on 11.03.23.
//

#include "VisualizationPanel.h"

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

    mFileTempo = std::make_unique<juce::TextEditor>();
    mFileTempo->setInputRestrictions(6, "0123456789.");
    mFileTempo->setMultiLine(false, false);
    mFileTempo->setReadOnly(false);

    mFileTempo->setFont(UIFonts::get().LABEL_FONT());
    mFileTempo->setJustification(juce::Justification::centred);

    mFileTempo->setColour(TextEditor::backgroundColourId, juce::Colours::transparentWhite);
    mFileTempo->setColour(TextEditor::textColourId, BLACK);
    mFileTempo->setColour(TextEditor::outlineColourId, juce::Colours::lightgrey);
    mFileTempo->setColour(TextEditor::focusedOutlineColourId, juce::Colours::grey);
    mFileTempo->onReturnKey = [this]() { mFileTempo->giveAwayKeyboardFocus(); };
    mFileTempo->onEscapeKey = [this]() { mFileTempo->giveAwayKeyboardFocus(); };
    mFileTempo->onFocusLost = [this]() {
        double tempo = jlimit(5.0, 900.0, mFileTempo->getText().getDoubleValue());
        String correct_tempo_str = String(tempo);
        correct_tempo_str = correct_tempo_str.substring(0, jmin(correct_tempo_str.length(), 6));
        mFileTempo->setText(correct_tempo_str);
        mProcessor->setMidiFileTempo(tempo);
    };
    mFileTempo->onTextChange = [this]() {
        double tempo = jlimit(5.0, 900.0, mFileTempo->getText().getDoubleValue());
        mProcessor->setMidiFileTempo(tempo);
    };

    mFileTempo->setText(String(mProcessor->getMidiFileTempo()));
    addChildComponent(*mFileTempo);

    mAudioGainSlider.setSliderStyle(Slider::SliderStyle::LinearHorizontal);
    mAudioGainSlider.setTextBoxStyle(Slider::TextEntryBoxPosition::TextBoxLeft, true, 40, 20);
    mAudioGainSlider.setTextValueSuffix(" dB");
    mAudioGainSlider.setColour(Slider::ColourIds::textBoxTextColourId, BLACK);
    mAudioGainSlider.setColour(Slider::ColourIds::textBoxOutlineColourId, Colours::transparentWhite);
    // To also receive mouseExit callback from this slider
    mAudioGainSlider.addMouseListener(this, true);
    mAudioGainSliderAttachment = std::make_unique<SliderParameterAttachment>(
        *mProcessor->mTree.getParameter("AUDIO_LEVEL_DB"), mAudioGainSlider);

    addChildComponent(mAudioGainSlider);

    mMidiGainSlider.setSliderStyle(Slider::SliderStyle::LinearHorizontal);
    mMidiGainSlider.setTextBoxStyle(Slider::TextEntryBoxPosition::TextBoxLeft, true, 40, 20);
    mMidiGainSlider.setTextValueSuffix(" dB");
    mMidiGainSlider.setColour(Slider::ColourIds::textBoxTextColourId, BLACK);
    mMidiGainSlider.setColour(Slider::ColourIds::textBoxOutlineColourId, Colours::transparentWhite);
    // To also receive mouseExit callback from this slider
    mMidiGainSlider.addMouseListener(this, true);

    mMidiGainSliderAttachment =
        std::make_unique<SliderParameterAttachment>(*mProcessor->mTree.getParameter("MIDI_LEVEL_DB"), mMidiGainSlider);

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
    mFileTempo->setBounds(6, 55, 40, 17);

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
        g.setFont(UIFonts::get().LABEL_FONT());
        g.drawFittedText(
            "MIDI\nFILE\nTEMPO", Rectangle<int>(0, 0, KEYBOARD_WIDTH, 55), juce::Justification::centred, 3);
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

    mFileTempo->setText(String(mProcessor->getMidiFileTempo()), sendNotification);
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
