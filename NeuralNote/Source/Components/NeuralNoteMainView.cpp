//
// Created by Damien Ronssin on 06.03.23.
//

#include "NeuralNoteMainView.h"

NeuralNoteMainView::NeuralNoteMainView(NeuralNoteAudioProcessor& processor)
    : mProcessor(processor)
    , mVisualizationPanel(&processor)
    , mTranscriptionOptions(processor)
    , mNoteOptions(processor)
    , mQuantizePanel(processor)
{
    mProcessor.addListenerToStateValueTree(this);
    jassert(mProcessor.getValueTree().hasProperty(NnId::PlayheadCenteredId));

    mRecordButton = std::make_unique<DrawableButton>("RecordButton", DrawableButton::ButtonStyle::ImageRaw);
    mRecordButton->setClickingTogglesState(true);
    mRecordButton->setColour(DrawableButton::ColourIds::backgroundColourId, Colours::transparentBlack);
    mRecordButton->setColour(DrawableButton::ColourIds::backgroundOnColourId, Colours::transparentBlack);
    mRecordButton->setTooltip(NeuralNoteTooltips::record);

    auto record_off_drawable =
        Drawable::createFromImageData(BinaryData::recordingoff_svg, BinaryData::recordingoff_svgSize);
    auto record_on_drawable =
        Drawable::createFromImageData(BinaryData::recordingon_svg, BinaryData::recordingon_svgSize);

    mRecordButton->setImages(
        record_off_drawable.get(), nullptr, nullptr, nullptr, record_on_drawable.get(), nullptr, nullptr);

    mRecordButton->onClick = [this]() {
        bool is_on = mRecordButton->getToggleState();

        // Recording started
        if (is_on) {
            mProcessor.getSourceAudioManager()->startRecording();
        } else {
            // Recording has ended, set processor state to processing
            mProcessor.getSourceAudioManager()->stopRecording();
        }

        updateEnablements();
    };

    mRecordButton->setToggleState(mProcessor.getState() == Recording, NotificationType::dontSendNotification);

    addAndMakeVisible(*mRecordButton);

    mClearButton = std::make_unique<DrawableButton>("ClearButton", DrawableButton::ButtonStyle::ImageRaw);
    mClearButton->setClickingTogglesState(false);
    mClearButton->setColour(DrawableButton::ColourIds::backgroundColourId, Colours::transparentBlack);
    mClearButton->setColour(DrawableButton::ColourIds::backgroundOnColourId, Colours::transparentBlack);
    mClearButton->setTooltip(NeuralNoteTooltips::clear);

    auto bin_drawable = Drawable::createFromImageData(BinaryData::deleteicon_svg, BinaryData::deleteicon_svgSize);
    mClearButton->setImages(bin_drawable.get());

    mClearButton->onClick = [this]() {
        mProcessor.clear();
        mVisualizationPanel.clear();
        updateEnablements();
    };
    addAndMakeVisible(*mClearButton);

    mBackButton = std::make_unique<DrawableButton>("BackButton", DrawableButton::ButtonStyle::ImageRaw);
    mBackButton->setClickingTogglesState(false);
    mBackButton->setColour(DrawableButton::ColourIds::backgroundColourId, Colours::transparentBlack);
    mBackButton->setColour(DrawableButton::ColourIds::backgroundOnColourId, Colours::transparentBlack);
    auto back_icon_drawable = Drawable::createFromImageData(BinaryData::back_svg, BinaryData::back_svgSize);
    mBackButton->setImages(back_icon_drawable.get());
    mBackButton->onClick = [this]() {
        mProcessor.getPlayer()->reset();
        mPlayPauseButton->setToggleState(false, sendNotification);
        mVisualizationPanel.getAudioMidiViewport().setViewPositionProportionately(0, 0);
    };
    mBackButton->setTooltip(NeuralNoteTooltips::back);
    addAndMakeVisible(*mBackButton);

    mPlayPauseButton = std::make_unique<DrawableButton>("PlayPauseButton", DrawableButton::ButtonStyle::ImageRaw);
    mPlayPauseButton->setClickingTogglesState(true);
    mPlayPauseButton->setColour(DrawableButton::ColourIds::backgroundColourId, Colours::transparentBlack);
    mPlayPauseButton->setColour(DrawableButton::ColourIds::backgroundOnColourId, Colours::transparentBlack);
    auto play_icon_drawable = Drawable::createFromImageData(BinaryData::play_svg, BinaryData::play_svgSize);
    auto pause_icon_drawable = Drawable::createFromImageData(BinaryData::pause_svg, BinaryData::pause_svgSize);
    mPlayPauseButton->setImages(
        play_icon_drawable.get(), nullptr, nullptr, nullptr, pause_icon_drawable.get(), nullptr, nullptr, nullptr);

    mPlayPauseButton->onClick = [this]() {
        if (mProcessor.getState() == PopulatedAudioAndMidiRegions) {
            mProcessor.getPlayer()->setPlayingState(mPlayPauseButton->getToggleState());
        } else {
            mPlayPauseButton->setToggleState(false, sendNotification);
        }
    };
    mPlayPauseButton->setTooltip(NeuralNoteTooltips::play_pause);

    addAndMakeVisible(*mPlayPauseButton);

    mCenterButton = std::make_unique<DrawableButton>("PlayPauseButton", DrawableButton::ButtonStyle::ImageRaw);
    mCenterButton->setClickingTogglesState(true);
    mCenterButton->setColour(DrawableButton::ColourIds::backgroundColourId, Colours::transparentBlack);
    mCenterButton->setColour(DrawableButton::ColourIds::backgroundOnColourId, Colours::transparentBlack);
    auto center_icon_drawable_off =
        Drawable::createFromImageData(BinaryData::center_off_svg, BinaryData::center_off_svgSize);
    auto center_icon_drawable_on =
        Drawable::createFromImageData(BinaryData::center_on_svg, BinaryData::center_on_svgSize);
    mCenterButton->setImages(center_icon_drawable_off.get(),
                             nullptr,
                             nullptr,
                             nullptr,
                             center_icon_drawable_on.get(),
                             nullptr,
                             nullptr,
                             nullptr);
    mCenterButton->setTooltip(NeuralNoteTooltips::center);

    mCenterButton->getToggleStateValue().referTo(
        mProcessor.getValueTree().getPropertyAsValue(NnId::PlayheadCenteredId, nullptr));
    NeuralNoteMainView::valueTreePropertyChanged(mProcessor.getValueTree(), NnId::PlayheadCenteredId);
    addAndMakeVisible(*mCenterButton);

    mUpdateCheck = std::make_unique<UpdateCheck>();

    mSettingsButton = std::make_unique<DrawableButton>("SettingsButton", DrawableButton::ButtonStyle::ImageRaw);
    mSettingsButton->setClickingTogglesState(false);
    mSettingsButton->setColour(DrawableButton::ColourIds::backgroundColourId, TRANSPARENT);
    mSettingsButton->setColour(DrawableButton::ColourIds::backgroundOnColourId, BLACK);
    auto settings_icon_drawable = Drawable::createFromImageData(BinaryData::settings_svg, BinaryData::settings_svgSize);
    mSettingsButton->setImages(settings_icon_drawable.get());
    addAndMakeVisible(mSettingsButton.get());

    mSettingsMenu = std::make_unique<PopupMenu>();

    // Midi out
    int item_id = 0;
    auto midi_out_item = PopupMenu::Item("MIDI Out");
    midi_out_item.setID(++item_id);
    midi_out_item.setEnabled(true);
    mSettingsMenuItemsShouldBeTicked.emplace_back(midi_out_item.itemID, [this] {
        return static_cast<bool>(mProcessor.getValueTree().getProperty(NnId::MidiOut));
    });

    midi_out_item.setTicked(mSettingsMenuItemsShouldBeTicked.back().second());
    auto midi_out_action = [this] {
        bool midi_out_enabled = mProcessor.getValueTree().getProperty(NnId::MidiOut);
        mProcessor.getValueTree().setProperty(NnId::MidiOut, !midi_out_enabled, nullptr);
        _updateSettingsMenuTicks();
    };

    midi_out_item = midi_out_item.setAction(midi_out_action);
    mSettingsMenu->addItem(midi_out_item);

    // Reset zoom
    auto reset_zoom_item = PopupMenu::Item("Reset Zoom");
    reset_zoom_item.setID(++item_id);
    reset_zoom_item.setTicked(false);
    auto reset_zoom_action = [this] { mProcessor.getValueTree().setProperty(NnId::ZoomLevelId, 1.0, nullptr); };
    reset_zoom_item.setAction(reset_zoom_action);
    mSettingsMenu->addItem(reset_zoom_item);

    // Tooltip visibility
    auto tooltip_visibility_item = PopupMenu::Item("Show Tooltips");
    tooltip_visibility_item.setID(++item_id);
    tooltip_visibility_item.setEnabled(true);
    mSettingsMenuItemsShouldBeTicked.emplace_back(tooltip_visibility_item.itemID, [this] {
        return static_cast<bool>(mProcessor.getValueTree().getProperty(NnId::TooltipVisibleId));
    });

    tooltip_visibility_item.setTicked(mSettingsMenuItemsShouldBeTicked.back().second());
    auto tooltip_visibility_action = [this] {
        bool tooltip_visibility = mProcessor.getValueTree().getProperty(NnId::TooltipVisibleId);
        mProcessor.getValueTree().setPropertyExcludingListener(
            this, NnId::TooltipVisibleId, !tooltip_visibility, nullptr);
        _updateTooltipVisibility();
        _updateSettingsMenuTicks();
    };
    tooltip_visibility_item.setAction(tooltip_visibility_action);
    mSettingsMenu->addItem(tooltip_visibility_item);

    // Check for updates
    auto check_updates_item = PopupMenu::Item("Check for updates");
    check_updates_item.setID(++item_id);
    check_updates_item.setEnabled(true);
    check_updates_item.setTicked(false);
    check_updates_item.setAction([this] { mUpdateCheck->checkForUpdate(true); });
    mSettingsMenu->addSeparator();
    mSettingsMenu->addItem(check_updates_item);

    mPopupMenuLookAndFeel = std::make_unique<PopupMenuLookAndFeel>();
    mPopupMenuLookAndFeel->setColour(PopupMenu::ColourIds::backgroundColourId, WHITE_SOLID);
    mPopupMenuLookAndFeel->setColour(PopupMenu::ColourIds::textColourId, BLACK);
    mSettingsMenu->setLookAndFeel(mPopupMenuLookAndFeel.get());

    mSettingsButton->onClick = [this] {
        _updateSettingsMenuTicks();
        PopupMenu::Options options;
        options = options.withTargetComponent(mSettingsButton.get());

        mSettingsMenu->showMenuAsync(options);
    };

    mMuteButton = std::make_unique<DrawableButton>("MuteButton", DrawableButton::ButtonStyle::ImageRaw);
    mMuteButton->setClickingTogglesState(true);
    mMuteButton->setColour(DrawableButton::ColourIds::backgroundColourId, Colours::transparentBlack);
    mMuteButton->setColour(DrawableButton::ColourIds::backgroundOnColourId, Colours::transparentBlack);

    auto mute_on_drawable = Drawable::createFromImageData(BinaryData::mute_svg, BinaryData::mute_svgSize);
    auto mute_off_drawable = Drawable::createFromImageData(BinaryData::unmute_svg, BinaryData::unmute_svgSize);

    mMuteButton->setImages(
        mute_off_drawable.get(), nullptr, nullptr, nullptr, mute_on_drawable.get(), nullptr, nullptr);
    mMuteButton->setClickingTogglesState(true);
    mMuteButton->setTooltip(NeuralNoteTooltips::mute);

    mMuteButtonAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(
        mProcessor.getAPVTS(), ParameterHelpers::getIdStr(ParameterHelpers::MuteId), *mMuteButton);
    addAndMakeVisible(*mMuteButton);

    addAndMakeVisible(mVisualizationPanel);
    addAndMakeVisible(mTranscriptionOptions);
    addAndMakeVisible(mNoteOptions);
    addAndMakeVisible(mQuantizePanel);

    mBackgroundImage = ImageCache::getFromMemory(BinaryData::background_png, BinaryData::background_pngSize)
                           .rescaled(1000, 640, Graphics::ResamplingQuality::highResamplingQuality);

    _updateTooltipVisibility();

    setWantsKeyboardFocus(true);
    mPlayPauseButton->setWantsKeyboardFocus(false);
    mBackButton->setWantsKeyboardFocus(false);
    mRecordButton->setWantsKeyboardFocus(false);
    mCenterButton->setWantsKeyboardFocus(false);
    mSettingsButton->setWantsKeyboardFocus(false);
    mSettingsButton->setTooltip(NeuralNoteTooltips::settings);

    updateEnablements();

    addChildComponent(mUpdateCheck.get());
    mUpdateCheck->checkForUpdate(false);

    startTimerHz(30);
}

NeuralNoteMainView::~NeuralNoteMainView()
{
    mProcessor.removeListenerFromStateValueTree(this);
    LookAndFeel::setDefaultLookAndFeel(nullptr);
}

void NeuralNoteMainView::resized()
{
    mRecordButton->setBounds(537, 43, 35, 35);
    mClearButton->setBounds(589, 43, 35, 35);

    mBackButton->setBounds(682, 43, 35, 35);
    mPlayPauseButton->setBounds(734, 43, 35, 35);
    mCenterButton->setBounds(786, 43, 35, 35);
    mSettingsButton->setBounds(838, 43, 35, 35);

    mMuteButton->setBounds(931, 43, 35, 35);

    mVisualizationPanel.setBounds(328, 120, 642, 491);
    mTranscriptionOptions.setBounds(29, 120, 274, 190);
    mNoteOptions.setBounds(29, 334, 274, 133);
    mQuantizePanel.setBounds(29, 491, 274, 120);

    mUpdateCheck->setBounds(680, 615, 290, 20);
}

void NeuralNoteMainView::paint(Graphics& g)
{
    g.drawImageAt(mBackgroundImage, 0, 0);
}

void NeuralNoteMainView::timerCallback()
{
    auto processor_state = mProcessor.getState();
    if (mRecordButton->getToggleState() && processor_state != Recording) {
        mRecordButton->setToggleState(false, sendNotification);
        updateEnablements();
    }

    if (mPlayPauseButton->getToggleState() != mProcessor.getPlayer()->isPlaying()) {
        mPlayPauseButton->setToggleState(mProcessor.getPlayer()->isPlaying(), sendNotification);
    }

    if (mPrevState != processor_state) {
        mPrevState = processor_state;
        updateEnablements();
    }
}

void NeuralNoteMainView::repaintPianoRoll()
{
    mVisualizationPanel.repaintPianoRoll();
}

bool NeuralNoteMainView::keyPressed(const KeyPress& key)
{
    if (key == KeyPress(KeyPress::spaceKey, ModifierKeys::shiftModifier, 0)) {
        mBackButton->triggerClick();
        return true;
    }

    if (key == KeyPress::spaceKey) {
        mPlayPauseButton->triggerClick();
        return true;
    }

    if (key == KeyPress(KeyPress::backspaceKey, ModifierKeys::shiftModifier, 0)) {
        mClearButton->triggerClick();
        return true;
    }

    if (key == KeyPress('r', juce::ModifierKeys::noModifiers, 0)) {
        mRecordButton->triggerClick();
        return true;
    }

    if (key == KeyPress('m', juce::ModifierKeys::noModifiers, 0)) {
        mMuteButton->triggerClick();
        return true;
    }

    if (key == KeyPress('c', juce::ModifierKeys::noModifiers, 0)) {
        mCenterButton->triggerClick();
        return true;
    }

    return false;
}

void NeuralNoteMainView::updateEnablements()
{
    auto current_state = mProcessor.getState();
    mPrevState = current_state;

    if (current_state == EmptyAudioAndMidiRegions) {
        mRecordButton->setEnabled(true);
        mClearButton->setEnabled(false);
        mPlayPauseButton->setEnabled(false);
        mBackButton->setEnabled(false);
        mCenterButton->setEnabled(false);
    } else if (current_state == Recording) {
        mRecordButton->setEnabled(true);
        mClearButton->setEnabled(false);
        mPlayPauseButton->setEnabled(false);
        mBackButton->setEnabled(false);
        mCenterButton->setEnabled(false);
    } else if (current_state == Processing) {
        mRecordButton->setEnabled(false);
        mClearButton->setEnabled(false);
        mPlayPauseButton->setEnabled(false);
        mBackButton->setEnabled(false);
        mCenterButton->setEnabled(false);
    } else if (current_state == PopulatedAudioAndMidiRegions) {
        mRecordButton->setEnabled(false);
        mClearButton->setEnabled(true);
        mPlayPauseButton->setEnabled(true);
        mBackButton->setEnabled(true);
        mCenterButton->setEnabled(true);
        mVisualizationPanel.setMidiFileDragComponentVisible();
    }

    repaint();
}

void NeuralNoteMainView::valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property)
{
    if (property == NnId::PlayheadCenteredId) {
        bool should_center = treeWhosePropertyHasChanged.getProperty(property);
        mCenterButton->setToggleState(should_center, sendNotification);
        mVisualizationPanel.getCombinedAudioMidiRegion().setCenterView(should_center);
    }

    if (property == NnId::TooltipVisibleId) {
        _updateTooltipVisibility();
    }
}

void NeuralNoteMainView::_updateSettingsMenuTicks()
{
    std::vector<int> ticked_items;

    for (auto& [item_id, should_be_ticked_lambda]: mSettingsMenuItemsShouldBeTicked) {
        if (should_be_ticked_lambda != nullptr) {
            if (should_be_ticked_lambda()) {
                ticked_items.emplace_back(item_id);
            }
        }
    }

    for (PopupMenu::MenuItemIterator iterator(*mSettingsMenu, true); iterator.next();) {
        auto& item = iterator.getItem();
        bool is_ticked = std::find(ticked_items.begin(), ticked_items.end(), item.itemID) != ticked_items.end();
        item.setTicked(is_ticked);
    }
}

void NeuralNoteMainView::_updateTooltipVisibility()
{
    if (mProcessor.getValueTree().getProperty(NnId::TooltipVisibleId, true)) {
        if (mTooltipWindow == nullptr) {
            mTooltipWindow = std::make_unique<TooltipWindow>(this, 800);
            mTooltipWindow->setOpaque(false);
        }
    } else {
        mTooltipWindow = nullptr;
    }
}
