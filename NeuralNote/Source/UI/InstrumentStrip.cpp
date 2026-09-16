//
// Created by Damien Ronssin on 07.08.26.
//

#include "InstrumentStrip.h"

#include "GainConstants.h"
#include "NnFonts.h"
#include "NnLook.h"

namespace
{
constexpr int PADDING_SIDE = 14;
constexpr int PADDING_TOP = 11;
constexpr int ROW_HEIGHT = 22;
constexpr int TOGGLE_WIDTH = 20;
constexpr int TOGGLE_HEIGHT = 18;
constexpr int TOGGLE_GAP = 3;
constexpr int FADER_TOP_GAP = 9;
constexpr int FADER_HEIGHT = 11;
constexpr int VALUE_WIDTH = 30;
constexpr int VALUE_GAP = 9;
constexpr int CHIP_CORNER = 5;
} // namespace

InstrumentStrip::InstrumentStrip(InstrumentMixer& inMixer, int inProgram)
    : mMixer(inMixer)
    , mProgram(inProgram)
{
    auto setup_toggle = [this](NnFlatButton& button, juce::Colour inOnBackground, juce::Colour inOnText) {
        button.setClickingTogglesState(true);
        button.setLabel(button.getName(), nn::fonts::metaStrong());
        button.setCornerRadius(4.0f);
        button.setColour(NnFlatButton::backgroundColourId, nn::colours::bgControlSubtle);
        button.setColour(NnFlatButton::backgroundOnColourId, inOnBackground);
        button.setColour(NnFlatButton::textColourId, nn::colours::textDim);
        button.setColour(NnFlatButton::textOnColourId, inOnText);
        button.setWantsKeyboardFocus(false);
        addAndMakeVisible(button);
    };

    setup_toggle(mMuteButton, nn::colours::bgMuteActive, nn::colours::warn);
    setup_toggle(mSoloButton, nn::colours::soloButtonBg(), nn::colours::rec);

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
    mFader.setColour(NnFlatSlider::fillColourId, muted ? nn::colours::faderFillMuted : mEntry.colour.withAlpha(0.85f));
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

    if (soloed) {
        g.fillAll(nn::colours::soloRowTint());
    }

    nn::drawBottomBorder(g, getLocalBounds(), nn::colours::divRow);

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
    g.setFont(nn::fonts::mono(8.0f, 600));
    g.drawText(mEntry.abbreviation, chip.toNearestInt(), juce::Justification::centred);

    // The name and the meta line share the identity row, split at the name's own line height.
    auto text_area = identity_row.withTrimmedLeft(nn::metrics::stripTextInset - nn::metrics::stripChipSize)
                         .withTrimmedRight(2 * TOGGLE_WIDTH + TOGGLE_GAP + 8);

    const auto name_font = nn::fonts::instrumentName();
    auto name_area = text_area.removeFromTop(juce::roundToInt(name_font.getHeight()));

    g.setColour((muted ? nn::colours::textLabel : nn::colours::textStrong).withMultipliedAlpha(alpha));
    g.setFont(name_font);
    g.drawText(mEntry.name, name_area, juce::Justification::centredLeft, true);

    if (muted) {
        const int name_width =
            juce::jmin(name_area.getWidth(), juce::GlyphArrangement::getStringWidthInt(name_font, mEntry.name));

        g.fillRect(name_area.getX(), name_area.getCentreY(), name_width, 1);
    }

    text_area.removeFromTop(2);

    g.setColour(nn::colours::textFaintest.withMultipliedAlpha(alpha));
    g.setFont(nn::fonts::meta());

    // Drums have no pitch range to report: their key numbers name pieces of a kit, not notes.
    const juce::String separator = " " + nn::separatorDot() + " ";
    juce::String meta;

    if (!mEntry.hasNotes()) {
        meta = "selected" + separator + "not transcribed yet";
    } else if (mEntry.program == msl::DRUM_PROGRAM) {
        meta = juce::String(mEntry.noteCount) + " hits" + separator + "kit map";
    } else {
        meta = juce::String(mEntry.noteCount) + " notes" + separator + nn::midiNoteName(mEntry.lowestPitch) + "-"
               + nn::midiNoteName(mEntry.highestPitch);
    }

    g.drawText(meta, text_area, juce::Justification::centredLeft, true);

    bounds.removeFromTop(FADER_TOP_GAP);

    auto value_area = bounds.removeFromTop(FADER_HEIGHT).removeFromRight(VALUE_WIDTH);

    g.setColour(nn::colours::textDim.withMultipliedAlpha(alpha));
    g.setFont(nn::fonts::meta());
    g.drawText(juce::String(mMixer.getGainDb(mProgram), 1), value_area, juce::Justification::centredRight);
}
