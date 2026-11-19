//
// Created by Damien Ronssin on 07.08.26.
//

#include "InstrumentStrip.h"

#include "GainConstants.h"
#include "NnFonts.h"
#include "NnLook.h"

namespace
{
constexpr int PADDING_SIDE = 16;
constexpr int PADDING_TOP = 12;
constexpr int ROW_HEIGHT = 26;
constexpr int TOGGLE_WIDTH = 26;
constexpr int TOGGLE_HEIGHT = 24;
constexpr int TOGGLE_GAP = 5;
constexpr int FADER_TOP_GAP = 10;
constexpr int FADER_HEIGHT = 16;
constexpr int VALUE_WIDTH = 34;
constexpr int VALUE_GAP = 10;
constexpr int CHIP_CORNER = 6;
constexpr int META_GAP = 2;
} // namespace

InstrumentStrip::InstrumentStrip(InstrumentMixer& inMixer, int inProgram)
    : mMixer(inMixer)
    , mProgram(inProgram)
{
    auto setup_toggle = [this](NnFlatButton& button, juce::Colour inOnBackground, juce::Colour inOnText) {
        button.setClickingTogglesState(true);
        button.setLabel(button.getName(), nn::fonts::buttonLabel());
        button.setCornerRadius(4.0f);
        button.setColour(NnFlatButton::backgroundColourId, nn::colours::bgControlSubtle);
        button.setColour(NnFlatButton::backgroundOnColourId, inOnBackground);
        button.setColour(NnFlatButton::textColourId, nn::colours::textDim);
        button.setColour(NnFlatButton::textOnColourId, inOnText);
        button.setWantsKeyboardFocus(false);
        addAndMakeVisible(button);
    };

    setup_toggle(mMuteButton, nn::colours::muteFill(), nn::colours::warn);
    setup_toggle(mSoloButton, nn::colours::accentFillToggle(), nn::colours::accentText);

    mMuteButton.setTooltip("Mute this instrument");
    mMuteButton.onClick = [this] { mMixer.setMuted(mProgram, mMuteButton.getToggleState()); };

    mSoloButton.setTooltip("Solo this instrument");
    mSoloButton.onClick = [this] { mMixer.setSoloed(mProgram, mSoloButton.getToggleState()); };

    mFader.setRange(MIN_INF_GAIN_DB, MAX_GAIN_DB, 0.1);
    mFader.setDoubleClickReturnValue(true, 0.0);
    mFader.setTooltip("Level for this instrument");
    // Repainted here rather than off a mixer change message: gain does not broadcast, and this
    // strip's dB readout is the only thing in the window that draws it.
    mFader.onValueChange = [this] {
        mMixer.setGainDb(mProgram, static_cast<float>(mFader.getValue()));
        repaint();
    };
    addAndMakeVisible(mFader);

    mMeter.setColour(NnLevelMeter::unlitColourId, nn::colours::meterUnlitStrip);
    addAndMakeVisible(mMeter);

    refreshFromMixer();
}

void InstrumentStrip::setEntry(const InstrumentEntry& inEntry)
{
    jassert(inEntry.program == mProgram);

    if (inEntry == mEntry) {
        return;
    }

    mEntry = inEntry;

    _updateAppearance();
    repaint();
}

void InstrumentStrip::setTranscriptionFinished(bool inIsFinished)
{
    if (inIsFinished != mTranscriptionFinished) {
        mTranscriptionFinished = inIsFinished;
        repaint();
    }
}

void InstrumentStrip::refreshFromMixer()
{
    mMuteButton.setToggleState(mMixer.isMuted(mProgram), juce::dontSendNotification);
    mSoloButton.setToggleState(mMixer.isSoloed(mProgram), juce::dontSendNotification);
    mFader.setValue(mMixer.getGainDb(mProgram), juce::dontSendNotification);

    _updateAppearance();
    repaint();
}

void InstrumentStrip::_updateAppearance()
{
    const bool muted = mMixer.isMuted(mProgram);

    // A strip the instrument picker put here has nothing to mix yet, so its controls are inert
    // until the next run gives it notes.
    mMuteButton.setEnabled(mEntry.hasNotes());
    mSoloButton.setEnabled(mEntry.hasNotes());
    mFader.setEnabled(mEntry.hasNotes());

    // Muted, the fader loses its instrument colour rather than only dimming: the whole strip is
    // already drawn at half opacity, and a dimmed hue on a dimmed strip stops reading as "off".
    mFader.setColour(NnFlatSlider::fillColourId,
                     muted ? nn::colours::faderFillMuted : nn::colours::faderFill(mEntry.colour));
    mFader.setColour(NnFlatSlider::thumbColourId, muted ? nn::colours::faderThumbMuted : nn::colours::faderThumb);

    // paint() cannot reach the children, so the strip's muted opacity is applied to them here.
    const float alpha = muted ? nn::MUTED_ALPHA : 1.0f;

    mMuteButton.setAlpha(alpha);
    mSoloButton.setAlpha(alpha);
    mFader.setAlpha(alpha);

    // The meter goes unlit on its own, being fed post-fader; this is for the 60 ms it spends
    // draining, and for its unlit track afterwards.
    mMeter.setAlpha(alpha);
}

void InstrumentStrip::resized()
{
    auto bounds = getLocalBounds().reduced(PADDING_SIDE, 0);
    bounds.removeFromTop(PADDING_TOP);

    auto identity_row = bounds.removeFromTop(ROW_HEIGHT);

    auto toggles = identity_row.removeFromRight(2 * TOGGLE_WIDTH + TOGGLE_GAP)
                       .withSizeKeepingCentre(2 * TOGGLE_WIDTH + TOGGLE_GAP, TOGGLE_HEIGHT);
    mMuteButton.setBounds(toggles.removeFromLeft(TOGGLE_WIDTH));
    toggles.removeFromLeft(TOGGLE_GAP);
    mSoloButton.setBounds(toggles);

    bounds.removeFromTop(FADER_TOP_GAP);

    auto fader_row = bounds.removeFromTop(FADER_HEIGHT);
    fader_row.removeFromLeft(nn::metrics::stripTextInset);
    fader_row.removeFromRight(VALUE_WIDTH + VALUE_GAP);
    mFader.setBounds(fader_row);

    bounds.removeFromTop(nn::metrics::stripMeterTopGap);

    // The fader's own insets, so the two line up under the name.
    auto meter_row = bounds.removeFromTop(nn::metrics::stripMeterHeight);
    meter_row.removeFromLeft(nn::metrics::stripTextInset);
    meter_row.removeFromRight(VALUE_WIDTH + VALUE_GAP);
    mMeter.setBounds(meter_row);
}

void InstrumentStrip::paint(juce::Graphics& g)
{
    const bool muted = mMixer.isMuted(mProgram);
    const bool soloed = mMixer.isSoloed(mProgram);

    // Each instrument is presented as a compact floating mixer card instead of a flat list row.
    // Keep the cards inside the viewport's spacing so the sidebar reads as a layered surface.
    const auto card = getLocalBounds().toFloat().reduced(7.0f, 3.0f);
    g.setColour(juce::Colours::black.withAlpha(0.025f));
    g.fillRoundedRectangle(card.translated(0.0f, 2.0f), 10.0f);
    g.setColour(soloed ? nn::colours::soloRowTint() : nn::colours::bgControl);
    g.fillRoundedRectangle(card, 10.0f);
    g.setColour(soloed ? nn::colours::accent.withAlpha(0.32f) : nn::colours::divSoft.withAlpha(0.65f));
    g.drawRoundedRectangle(card.reduced(0.5f), 10.0f, 1.0f);

    if (soloed) {
        g.setColour(nn::colours::accent);
        g.fillRoundedRectangle(juce::Rectangle<float>(3.0f, card.getHeight() - 16.0f)
                                  .withCentre({card.getX() + 2.0f, card.getCentreY()}),
                              1.5f);
    }

    const float alpha = muted ? nn::MUTED_ALPHA : 1.0f;

    auto bounds = getLocalBounds().reduced(PADDING_SIDE, 0);
    bounds.removeFromTop(PADDING_TOP);

    auto identity_row = bounds.removeFromTop(ROW_HEIGHT);

    const auto chip = identity_row.removeFromLeft(nn::metrics::stripChipSize).toFloat();

    g.setColour(nn::colours::chipFill(mEntry.colour).withMultipliedAlpha(alpha));
    g.fillRoundedRectangle(chip, CHIP_CORNER);

    g.setColour(nn::colours::chipBorder(mEntry.colour).withMultipliedAlpha(alpha));
    g.drawRoundedRectangle(chip.reduced(0.5f), CHIP_CORNER, 1.0f);

    g.setColour(mEntry.colour.withMultipliedAlpha(alpha));
    g.setFont(nn::fonts::mono(9.0f, 600));
    g.drawText(mEntry.abbreviation, chip.toNearestInt(), juce::Justification::centred);

    // The meta line sits under the name, each at its font's own height, and runs past the identity
    // row into the gap above the fader.
    auto text_area = identity_row.withTrimmedLeft(nn::metrics::stripTextInset - nn::metrics::stripChipSize)
                         .withTrimmedRight(2 * TOGGLE_WIDTH + TOGGLE_GAP + 8);

    const auto name_font = nn::fonts::instrumentName();
    auto name_area = text_area.removeFromTop(juce::roundToInt(name_font.getHeight()));

    g.setColour((muted ? nn::colours::textLabel : nn::colours::textStrong).withMultipliedAlpha(alpha));
    g.setFont(name_font);
    g.drawText(mEntry.name, name_area, juce::Justification::centredLeft, true);

    const auto meta_font = nn::fonts::meta();
    const auto meta_area = text_area.withTrimmedTop(META_GAP).withHeight(juce::roundToInt(meta_font.getHeight()));

    // Drums have no pitch range to report: their key numbers name pieces of a kit, not notes.
    const juce::String separator = " " + nn::separatorDot() + " ";
    juce::String meta;

    if (!mEntry.hasNotes()) {
        meta = mTranscriptionFinished ? "no notes" : juce::String();
    } else if (mEntry.program == msl::DRUM_PROGRAM) {
        meta = juce::String(mEntry.noteCount) + " hits" + separator + "kit map";
    } else {
        meta = juce::String(mEntry.noteCount) + " notes" + separator + nn::midiNoteName(mEntry.lowestPitch) + "-"
               + nn::midiNoteName(mEntry.highestPitch);
    }

    g.setColour(nn::colours::textFainter.withMultipliedAlpha(alpha));
    g.setFont(meta_font);
    g.drawText(meta, meta_area, juce::Justification::centredLeft, true);

    bounds.removeFromTop(FADER_TOP_GAP);

    auto value_area = bounds.removeFromTop(FADER_HEIGHT).removeFromRight(VALUE_WIDTH);

    g.setColour(nn::colours::textDim.withMultipliedAlpha(alpha));
    g.setFont(nn::fonts::meta());
    g.drawText(juce::String(mMixer.getGainDb(mProgram), 1), value_area, juce::Justification::centredRight);
}
