//
// Created by Damien Ronssin on 07.08.26.
//

#include "TopBar.h"

#include "NeuralNoteTooltips.h"
#include "NNFileUtils.h"
#include "NnGlobalSettings.h"
#include "NnFonts.h"
#include "NnIcons.h"
#include "NnLook.h"
#include "PluginProcessor.h"

namespace
{
constexpr int PADDING_LEFT = 18;
constexpr int PADDING_RIGHT = 14;

// Tighter than nn::metrics::controlGap: the transport's buttons draw no box when idle, so their
// icons already sit apart.
constexpr int TRANSPORT_GAP = 2;

constexpr int PILL_PADDING = 12;
constexpr int PILL_GAP = 9;
constexpr int MIX_TRACK_WIDTH = 86;
constexpr int VOLUME_TRACK_WIDTH = 74;
// Wide enough for -36.0, and what the instrument strips give the same readout.
constexpr int VOLUME_VALUE_WIDTH = 30;
constexpr int SPEAKER_ICON_SIZE = 13;
constexpr int SETTINGS_WIDTH = 32;

constexpr float LABEL_TRACKING = 0.08f;

constexpr int MODEL_BUTTON_PADDING = 11;
constexpr float MODEL_LABEL_TRACKING = 0.09f;

juce::String modelButtonLabel(const char* inModelName)
{
    return ("Model: " + juce::String(inModelName)).toUpperCase();
}

/** Sized for the longest label it can carry, so changing model does not move the bar around it. */
int modelButtonWidth()
{
    float widest = nn::trackedTextWidth(modelButtonLabel("None"), nn::fonts::sectionHeader(), MODEL_LABEL_TRACKING);

    for (const ModelSize size: ALL_MODEL_SIZES) {
        widest = std::max(widest,
                          nn::trackedTextWidth(modelButtonLabel(modelSizeToDisplayName(size)),
                                               nn::fonts::sectionHeader(),
                                               MODEL_LABEL_TRACKING));
    }

    return 2 * MODEL_BUTTON_PADDING + juce::roundToInt(std::ceil(widest));
}

void paintWordmark(juce::Graphics& g, juce::Rectangle<int> inBounds)
{
    const auto centre_y = static_cast<float>(inBounds.getCentreY());

    g.setColour(nn::colours::accent);
    g.fillRoundedRectangle(
        juce::Rectangle<float>(9.0f, 9.0f).withCentre({static_cast<float>(inBounds.getX()) + 4.5f, centre_y}), 2.0f);

    const auto name_font = nn::fonts::wordmark();
    const float name_x = static_cast<float>(inBounds.getX()) + 9.0f + 9.0f;
    const float name_width = nn::trackedTextWidth("NEURALNOTE", name_font, 0.14f);

    g.setColour(nn::colours::textBright);
    nn::drawTrackedText(
        g,
        "NEURALNOTE",
        name_font,
        {name_x, static_cast<float>(inBounds.getY()), name_width, static_cast<float>(inBounds.getHeight())},
        juce::Justification::centredLeft,
        0.14f);

    // Nudged down against the wordmark's cap height, which is where the mockup sits it.
    g.setColour(nn::colours::textFaint);
    nn::drawTrackedText(g,
                        "v2",
                        nn::fonts::wordmarkVersion(),
                        {name_x + name_width + 9.0f,
                         static_cast<float>(inBounds.getY()) + 2.0f,
                         30.0f,
                         static_cast<float>(inBounds.getHeight())},
                        juce::Justification::centredLeft,
                        0.06f);
}

} // namespace

TopBar::TopBar(NeuralNoteAudioProcessor& inProcessor)
    : mProcessor(inProcessor)
    , mTimeDisplay(&inProcessor)
{
    auto add_transport = [this](NnFlatButton& button, const juce::String& tooltip) {
        button.setColour(NnFlatButton::backgroundColourId, juce::Colours::transparentBlack);
        button.setTooltip(tooltip);
        button.setWantsKeyboardFocus(false);
        addAndMakeVisible(button);
    };

    mBackButton.setIcon(nn::icons::skipToStart, NnFlatButton::IconStyle::filled, 15.0f);
    mBackButton.setColour(NnFlatButton::iconColourId, nn::colours::textIcon);
    // onClick belongs to the main view: returning to the start also scrolls the timeline back,
    // which is not this component's to do.
    add_transport(mBackButton, NeuralNoteTooltips::back);

    // One button showing whichever icon is the action available now: pause while it plays.
    mPlayPauseButton.setClickingTogglesState(true);
    mPlayPauseButton.setIcon(nn::icons::play, NnFlatButton::IconStyle::filled, 15.0f);
    mPlayPauseButton.setColour(NnFlatButton::iconColourId, nn::colours::textPrimary);
    mPlayPauseButton.setColour(NnFlatButton::iconOnColourId, nn::colours::textPrimary);
    mPlayPauseButton.setColour(NnFlatButton::backgroundOnColourId, nn::colours::bgControlActive);
    mPlayPauseButton.onClick = [this] {
        const bool should_play = mPlayPauseButton.getToggleState();

        if (mProcessor.canPlay()) {
            mProcessor.getPlayer()->setPlayingState(should_play);
        } else {
            mPlayPauseButton.setToggleState(false, juce::dontSendNotification);
        }

        syncTransportToggles();
    };
    add_transport(mPlayPauseButton, NeuralNoteTooltips::play_pause);

    mLoopButton.setClickingTogglesState(true);
    mLoopButton.setIcon(nn::icons::loopStroked, NnFlatButton::IconStyle::stroked, 16.0f);
    mLoopButton.setOverlayIcon(nn::icons::loopHead);
    mLoopButton.setColour(NnFlatButton::iconColourId, nn::colours::textIcon);
    mLoopButton.setColour(NnFlatButton::iconOnColourId, nn::colours::accent);
    mLoopButton.setColour(NnFlatButton::backgroundOnColourId, nn::colours::accentFillActive());
    add_transport(mLoopButton, NeuralNoteTooltips::loop);

    mFollowButton.setClickingTogglesState(true);
    mFollowButton.setIcon(nn::icons::followPlayheadStroked, NnFlatButton::IconStyle::stroked, 16.0f);
    mFollowButton.setOverlayIcon(nn::icons::followPlayheadFlag);
    mFollowButton.setColour(NnFlatButton::iconColourId, nn::colours::textIcon);
    mFollowButton.setColour(NnFlatButton::iconOnColourId, nn::colours::accent);
    mFollowButton.setColour(NnFlatButton::backgroundOnColourId, nn::colours::accentFillActive());
    add_transport(mFollowButton, NeuralNoteTooltips::center);

    mRecordButton.setClickingTogglesState(true);
    mRecordButton.setIcon(nn::icons::record, NnFlatButton::IconStyle::filled, 16.0f);
    mRecordButton.setColour(NnFlatButton::iconColourId, nn::colours::recIdle);
    mRecordButton.setColour(NnFlatButton::iconOnColourId, nn::colours::rec);
    mRecordButton.setColour(NnFlatButton::backgroundOnColourId, nn::colours::activeFill(nn::colours::rec));
    mRecordButton.onClick = [this] {
        if (mRecordButton.getToggleState()) {
            mProcessor.getSourceAudioManager()->startRecording();
        } else {
            mProcessor.getSourceAudioManager()->stopRecording();
        }

        updateEnablements();
    };
    mRecordButton.setToggleState(mProcessor.getState() == Recording, juce::dontSendNotification);
    add_transport(mRecordButton, NeuralNoteTooltips::record);

    addAndMakeVisible(mTimeDisplay);

    // onClick belongs to the main view, which owns the panel this opens.
    mModelButton.setPadding(MODEL_BUTTON_PADDING, MODEL_BUTTON_PADDING, 7);
    mModelButton.setColour(NnFlatButton::backgroundColourId, nn::colours::bgControl);
    mModelButton.setColour(NnFlatButton::backgroundOnColourId, nn::colours::accentFillActive());
    mModelButton.setColour(NnFlatButton::textColourId, nn::colours::textButton);
    mModelButton.setColour(NnFlatButton::textOnColourId, nn::colours::accentText);
    mModelButton.setTooltip(NeuralNoteTooltips::model);
    mModelButton.setWantsKeyboardFocus(false);
    addAndMakeVisible(mModelButton);
    syncModelButton(false);

    mMixSlider.setColour(NnFlatSlider::trackColourId, nn::colours::faderTrackTop);
    mMixSlider.setColour(NnFlatSlider::fillColourId, nn::colours::mixSliderFill());
    mMixSlider.setTooltip("Balance between the source audio and the synthesised transcription");
    mMixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        mProcessor.getAPVTS(), ParameterHelpers::getIdStr(ParameterHelpers::MixId), mMixSlider);
    addAndMakeVisible(mMixSlider);

    mMasterGainSlider.setColour(NnFlatSlider::trackColourId, nn::colours::faderTrackTop);
    mMasterGainSlider.setColour(NnFlatSlider::fillColourId, nn::colours::volumeFill);
    mMasterGainSlider.setTooltip("Output level");
    // The pill paints the dB readout, so it has to repaint with the fader.
    mMasterGainSlider.onValueChange = [this] { repaint(); };
    mMasterGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        mProcessor.getAPVTS(), ParameterHelpers::getIdStr(ParameterHelpers::MasterGainId), mMasterGainSlider);
    addAndMakeVisible(mMasterGainSlider);

    mMuteButton.setClickingTogglesState(true);
    mMuteButton.setIcon(nn::icons::speakerMuted, NnFlatButton::IconStyle::filled, 14.0f);
    mMuteButton.setLabel("MUTE", nn::fonts::sectionHeader(), 0.09f);
    mMuteButton.setPadding(11, 11, 7);
    mMuteButton.setColour(NnFlatButton::backgroundColourId, nn::colours::bgControl);
    mMuteButton.setColour(NnFlatButton::backgroundOnColourId, nn::colours::muteFill());
    mMuteButton.setColour(NnFlatButton::iconColourId, nn::colours::textIcon);
    mMuteButton.setColour(NnFlatButton::iconOnColourId, nn::colours::warn);
    mMuteButton.setColour(NnFlatButton::textColourId, nn::colours::textIcon);
    mMuteButton.setColour(NnFlatButton::textOnColourId, nn::colours::warn);
    mMuteButton.setTooltip(NeuralNoteTooltips::mute);
    mMuteButton.setWantsKeyboardFocus(false);
    mMuteAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        mProcessor.getAPVTS(), ParameterHelpers::getIdStr(ParameterHelpers::MuteId), mMuteButton);
    addAndMakeVisible(mMuteButton);

    mSettingsButton.setIcon(nn::icons::settingsStroked, NnFlatButton::IconStyle::stroked, 14.0f);
    mSettingsButton.setColour(NnFlatButton::backgroundColourId, nn::colours::bgControl);
    mSettingsButton.setColour(NnFlatButton::iconColourId, nn::colours::textIcon);
    mSettingsButton.setTooltip(NeuralNoteTooltips::settings);
    mSettingsButton.setWantsKeyboardFocus(false);
    addAndMakeVisible(mSettingsButton);

    updateEnablements();
}

void TopBar::resized()
{
    // The wordmark sits over the sidebar. The four groups share the stretch over the content area,
    // from the sidebar's edge line to the right padding.
    auto bounds = getLocalBounds().withTrimmedBottom(1).withTrimmedRight(PADDING_RIGHT);
    bounds.setLeft(nn::metrics::sidebarWidth - 1);

    const auto label_font = nn::fonts::pillLabel();
    const int orig_width = juce::roundToInt(std::ceil(nn::trackedTextWidth("ORIG", label_font, LABEL_TRACKING)));
    const int midi_width = juce::roundToInt(std::ceil(nn::trackedTextWidth("MIDI", label_font, LABEL_TRACKING)));

    const int transport_width = 5 * nn::metrics::transportButtonW + 4 * TRANSPORT_GAP;
    const int time_width = TimeDisplay::getIdealWidth();
    const int model_width = modelButtonWidth();
    const int mix_width = 2 * PILL_PADDING + orig_width + PILL_GAP + MIX_TRACK_WIDTH + PILL_GAP + midi_width;
    const int volume_width =
        2 * PILL_PADDING + SPEAKER_ICON_SIZE + PILL_GAP + VOLUME_TRACK_WIDTH + PILL_GAP + VOLUME_VALUE_WIDTH;
    const int mute_width = mMuteButton.getIdealWidth();
    const int levels_width = mix_width + nn::metrics::controlGap + volume_width + nn::metrics::controlGap + mute_width
                             + nn::metrics::controlGap + SETTINGS_WIDTH;

    // What the groups leave over is split evenly either side of the three 1 px rules, the rounding
    // remainder a pixel at a time, so both ends of the bar land exactly on their edges.
    const int num_sides = 2 * static_cast<int>(mRuleXs.size());
    const int leftover = bounds.getWidth() - transport_width - time_width - model_width - levels_width
                         - static_cast<int>(mRuleXs.size());
    jassert(leftover >= num_sides);

    int side_index = 0;
    auto take_side = [&] {
        bounds.removeFromLeft(leftover / num_sides + (side_index++ < leftover % num_sides ? 1 : 0));
    };

    auto take = [&bounds](int inWidth, int inHeight = nn::metrics::controlHeight) {
        return bounds.removeFromLeft(inWidth).withSizeKeepingCentre(inWidth, inHeight);
    };

    auto take_rule = [&](std::size_t inIndex) {
        take_side();
        mRuleXs[inIndex] = bounds.removeFromLeft(1).getX();
        take_side();
    };

    auto transport = take(transport_width, nn::metrics::transportButtonH);

    for (NnFlatButton* button: {&mBackButton, &mPlayPauseButton, &mLoopButton, &mFollowButton, &mRecordButton}) {
        button->setBounds(transport.removeFromLeft(nn::metrics::transportButtonW));
        transport.removeFromLeft(TRANSPORT_GAP);
    }

    take_rule(0);
    mTimeDisplay.setBounds(take(time_width));

    take_rule(1);
    mModelButton.setBounds(take(model_width));

    take_rule(2);
    auto levels = take(levels_width);

    mMixPill = levels.removeFromLeft(mix_width);
    mMixSlider.setBounds(
        mMixPill.getX() + PILL_PADDING + orig_width + PILL_GAP, mMixPill.getY(), MIX_TRACK_WIDTH, mMixPill.getHeight());

    levels.removeFromLeft(nn::metrics::controlGap);
    mVolumePill = levels.removeFromLeft(volume_width);
    mMasterGainSlider.setBounds(mVolumePill.getX() + PILL_PADDING + SPEAKER_ICON_SIZE + PILL_GAP,
                                mVolumePill.getY(),
                                VOLUME_TRACK_WIDTH,
                                mVolumePill.getHeight());

    levels.removeFromLeft(nn::metrics::controlGap);
    mMuteButton.setBounds(levels.removeFromLeft(mute_width));

    levels.removeFromLeft(nn::metrics::controlGap);
    mSettingsButton.setBounds(levels);
}

void TopBar::paint(juce::Graphics& g)
{
    g.fillAll(nn::colours::bgTopBar);
    nn::drawBottomBorder(g, getLocalBounds(), nn::colours::divStrong);

    paintWordmark(g, getLocalBounds().withTrimmedLeft(PADDING_LEFT).withTrimmedBottom(1));

    const int rule_top = (getHeight() - 1 - nn::metrics::controlHeight) / 2;

    g.setColour(nn::colours::divStrong);

    for (const int x: mRuleXs) {
        g.fillRect(x, rule_top, 1, nn::metrics::controlHeight);
    }

    // The two level controls are held back until there is something to hear. Dimmed rather than
    // disabled: what they set survives a take being cleared, so they are never really unavailable.
    // The mix needs a transcription to balance against, which is a narrower thing than playback.
    _paintMixPill(g, _hasTranscription() ? 1.0f : nn::DISABLED_ALPHA);
    _paintVolumePill(g, mProcessor.canPlay() ? 1.0f : nn::DISABLED_ALPHA);
}

void TopBar::updateEnablements()
{
    const State state = mProcessor.getState();
    const bool can_play = mProcessor.canPlay();

    mBackButton.setEnabled(can_play);
    mPlayPauseButton.setEnabled(can_play);
    mLoopButton.setEnabled(can_play);
    mFollowButton.setEnabled(can_play);
    mRecordButton.setEnabled(state == EmptyAudioAndMidiRegions || state == Recording);

    mMasterGainSlider.setAlpha(can_play ? 1.0f : nn::DISABLED_ALPHA);

    _refreshMixAvailability();

    repaint();
}

void TopBar::syncTransportToggles()
{
    // Notes arrive chunk by chunk during a run, so the mix becomes available part-way through one
    // rather than at a state change. This is the only thing watching for that.
    _refreshMixAvailability();

    const bool is_playing = mProcessor.getPlayer()->isPlaying();

    if (mPlayPauseButton.getToggleState() != is_playing) {
        mPlayPauseButton.setToggleState(is_playing, juce::dontSendNotification);
    }

    // Guarded: this runs off a 20 Hz timer, and setIcon repaints whether or not the icon changed.
    if (mShowingPauseIcon != is_playing) {
        mShowingPauseIcon = is_playing;

        mPlayPauseButton.setIcon(is_playing ? nn::icons::pause : nn::icons::play,
                                 NnFlatButton::IconStyle::filled,
                                 is_playing ? 16.0f : 15.0f);
    }

    if (mRecordButton.getToggleState() && mProcessor.getState() != Recording) {
        mRecordButton.setToggleState(false, juce::sendNotification);
    }
}

void TopBar::_paintMixPill(juce::Graphics& g, float inAlpha) const
{
    if (mMixPill.isEmpty()) {
        return;
    }

    g.setColour(nn::colours::bgControl.withMultipliedAlpha(inAlpha));
    g.fillRoundedRectangle(mMixPill.toFloat(), static_cast<float>(nn::metrics::controlCorner));

    const auto font = nn::fonts::pillLabel();
    auto text_area = mMixPill.reduced(PILL_PADDING, 0).toFloat();

    g.setColour(nn::colours::textLabel.withMultipliedAlpha(inAlpha));
    nn::drawTrackedText(g, "ORIG", font, text_area, juce::Justification::centredLeft, LABEL_TRACKING);

    g.setColour(nn::colours::accentText.withMultipliedAlpha(inAlpha));
    nn::drawTrackedText(g, "MIDI", font, text_area, juce::Justification::centredRight, LABEL_TRACKING);
}

void TopBar::_paintVolumePill(juce::Graphics& g, float inAlpha) const
{
    if (mVolumePill.isEmpty()) {
        return;
    }

    g.setColour(nn::colours::bgControl.withMultipliedAlpha(inAlpha));
    g.fillRoundedRectangle(mVolumePill.toFloat(), static_cast<float>(nn::metrics::controlCorner));

    const auto icon_box =
        juce::Rectangle<float>(static_cast<float>(mVolumePill.getX() + PILL_PADDING),
                               static_cast<float>(mVolumePill.getCentreY()) - SPEAKER_ICON_SIZE / 2.0f,
                               SPEAKER_ICON_SIZE,
                               SPEAKER_ICON_SIZE);

    g.setColour(nn::colours::textIcon.withMultipliedAlpha(inAlpha));
    g.fillPath(nn::icons::speaker(icon_box));

    g.setColour(nn::colours::textLabel.withMultipliedAlpha(inAlpha));
    g.setFont(nn::fonts::meta());
    g.drawText(juce::String(mMasterGainSlider.getValue(), 1),
               mVolumePill.withTrimmedRight(PILL_PADDING).removeFromRight(VOLUME_VALUE_WIDTH),
               juce::Justification::centredRight);
}

bool TopBar::_hasTranscription() const
{
    return mProcessor.getInstrumentMixer()->getTotalNoteCount() > 0;
}

void TopBar::_refreshMixAvailability()
{
    const bool held = !_hasTranscription();

    if (mMixHeldToOriginal == held) {
        return;
    }

    mMixHeldToOriginal = held;

    mMixSlider.setAlpha(held ? nn::DISABLED_ALPHA : 1.0f);
    repaint();
}

void TopBar::syncModelButton(bool inModelPanelVisible)
{
    const std::optional<ModelSize> in_use = NNFileUtils::getInstalledModelSize(NnGlobalSettings::getModelSize());
    const juce::String label = modelButtonLabel(in_use.has_value() ? modelSizeToDisplayName(*in_use) : "None");

    if (label != mModelButtonLabel) {
        mModelButtonLabel = label;
        // The mute button's type, so the two labelled buttons of this bar read as a pair.
        mModelButton.setLabel(label, nn::fonts::sectionHeader(), MODEL_LABEL_TRACKING);
    }

    if (mModelButton.getToggleState() != inModelPanelVisible) {
        mModelButton.setToggleState(inModelPanelVisible, juce::dontSendNotification);
    }
}
