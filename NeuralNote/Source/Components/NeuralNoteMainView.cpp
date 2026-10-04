//
// Created by Damien Ronssin on 06.03.23.
//

#include "NeuralNoteMainView.h"

#include "ComputeDevices.h"
#include "MidiFileWriter.h"
#include "NeuralNoteTooltips.h"
#include "NNFileUtils.h"
#include "NnFonts.h"
#include "NnGlobalSettings.h"
#include "NnLook.h"
#include "PluginEditor.h"

namespace
{
/** Between the update notification and the status bar below it. */
constexpr int UPDATE_NOTIFICATION_GAP = 10;
} // namespace

NeuralNoteMainView::NeuralNoteMainView(NeuralNoteAudioProcessor& processor)
    : mProcessor(processor)
    , mTopBar(processor)
    , mSidebar(processor)
    , mVisualizationPanel(&processor)
    , mInstrumentMenu(processor)
{
    mProcessor.addListenerToStateValueTree(this);
    jassert(mProcessor.getValueTree().hasProperty(NnId::PlayheadCenteredId));

    addAndMakeVisible(mTopBar);
    addAndMakeVisible(mSidebar);
    addAndMakeVisible(mVisualizationPanel);

    addChildComponent(mInstrumentMenu);
    mInstrumentMenu.onDismiss = [this] { _setInstrumentMenuOpen(false); };

    // Only ever opens: while the menu is up its scrim covers the "+" as well, so a second click
    // there is the click that closes it.
    mSidebar.onAddInstrument = [this] { _setInstrumentMenuOpen(true); };

    mUpdateCheck = std::make_unique<UpdateCheck>();

    // Off this thread: the first listing initialises the GPU backends.
    ComputeDevices::prepare();

    mTopBar.getModelButton().onClick = [this] {
        mVisualizationPanel.setModelPanelOpen(!mVisualizationPanel.isModelPanelVisible());
        mTopBar.syncModelButton(mVisualizationPanel.isModelPanelVisible());
    };

    mTopBar.getSettingsButton().onClick = [this] {
        _buildSettingsMenu();
        mSettingsMenu->showMenuAsync(PopupMenu::Options().withTargetComponent(&mTopBar.getSettingsButton()));
    };

    mTopBar.getFollowButton().getToggleStateValue().referTo(
        mProcessor.getValueTree().getPropertyAsValue(NnId::PlayheadCenteredId, nullptr));
    NeuralNoteMainView::valueTreePropertyChanged(mProcessor.getValueTree(), NnId::PlayheadCenteredId);

    mTopBar.getLoopButton().getToggleStateValue().referTo(
        mProcessor.getValueTree().getPropertyAsValue(NnId::LoopEnabledId, nullptr));

    mTopBar.getBackButton().onClick = [this] {
        mProcessor.getPlayer()->returnToStart();
        mTopBar.syncTransportToggles();
        mVisualizationPanel.getAudioMidiViewport().setViewPositionProportionately(0, 0);
    };

    _updateTooltipVisibility();

    setWantsKeyboardFocus(true);

    updateEnablements();

    addChildComponent(mUpdateCheck.get());
    mUpdateCheck->checkForUpdate(false);

    // Only to notice the processor state and the transport changing under the UI. Nothing here
    // repaints unconditionally: everything that moves per frame is on a vblank callback of its own.
    startTimerHz(20);
}

NeuralNoteMainView::~NeuralNoteMainView()
{
    mProcessor.removeListenerFromStateValueTree(this);
}

void NeuralNoteMainView::resized()
{
    auto bounds = getLocalBounds();

    mTopBar.setBounds(bounds.removeFromTop(nn::metrics::topBarHeight));
    mSidebar.setBounds(bounds.removeFromLeft(nn::metrics::sidebarWidth));
    mVisualizationPanel.setBounds(bounds);

    // Just above the status bar. Wider than the panel needs -- it sizes itself to its message and
    // pins itself to the right of this, so what is set here is the room it has to grow into.
    mUpdateCheck->setBounds(bounds.getRight() - 460,
                            bounds.getBottom() - nn::metrics::statusBarHeight - UPDATE_NOTIFICATION_GAP
                                - nn::metrics::menuRowHeight,
                            460 - nn::metrics::menuPadX,
                            nn::metrics::menuRowHeight);

    mInstrumentMenu.setBounds(getLocalBounds());
    mInstrumentMenu.setPanelAnchor(mSidebar.getPosition() + mSidebar.getMenuAnchor());
}

void NeuralNoteMainView::_setInstrumentMenuOpen(bool inIsOpen)
{
    mInstrumentMenu.setVisible(inIsOpen);
    mSidebar.setMenuOpen(inIsOpen);

    if (inIsOpen) {
        // Above the update notification, which is added after it and would otherwise sit on top.
        mInstrumentMenu.toFront(false);
    }

    if (!inIsOpen) {
        // The transport shortcuts live here, and the menu took the focus to hear Escape.
        grabKeyboardFocus();
    }
}

void NeuralNoteMainView::paint(Graphics& g)
{
    g.fillAll(nn::colours::bgRoot);
}

void NeuralNoteMainView::timerCallback()
{
    mTopBar.syncTransportToggles();

    // The model panel changes what the button shows -- its cross, its selection -- without telling it.
    mTopBar.syncModelButton(mVisualizationPanel.isModelPanelVisible());

    const State processor_state = mProcessor.getState();

    if (mPrevState != processor_state) {
        mPrevState = processor_state;
        updateEnablements();
    }
}

void NeuralNoteMainView::repaintPianoRoll()
{
    mVisualizationPanel.repaintPianoRoll();
}

void NeuralNoteMainView::clear()
{
    jassert(MessageManager::getInstance()->isThisTheMessageThread());

    mVisualizationPanel.clear();
    updateEnablements();
}

bool NeuralNoteMainView::keyPressed(const KeyPress& key)
{
    if (key == KeyPress(KeyPress::spaceKey, ModifierKeys::shiftModifier, 0)) {
        mTopBar.getBackButton().triggerClick();
        return true;
    }

    if (key == KeyPress::spaceKey) {
        mTopBar.getPlayPauseButton().triggerClick();
        return true;
    }

    if (key == KeyPress(KeyPress::backspaceKey, ModifierKeys::shiftModifier, 0)) {
        mVisualizationPanel.getToolbar().getClearButton().triggerClick();
        return true;
    }

    if (key == KeyPress('r', juce::ModifierKeys::noModifiers, 0)) {
        mTopBar.getRecordButton().triggerClick();
        return true;
    }

    if (key == KeyPress('m', juce::ModifierKeys::noModifiers, 0)) {
        mTopBar.getMuteButton().triggerClick();
        return true;
    }

    if (key == KeyPress('c', juce::ModifierKeys::noModifiers, 0)) {
        mTopBar.getFollowButton().triggerClick();
        return true;
    }

    if (key == KeyPress('l', juce::ModifierKeys::noModifiers, 0)) {
        mTopBar.getLoopButton().triggerClick();
        return true;
    }

    return false;
}

void NeuralNoteMainView::updateEnablements()
{
    mPrevState = mProcessor.getState();

    mTopBar.updateEnablements();
    mTopBar.syncTransportToggles();
    mSidebar.updateEnablements();
    mVisualizationPanel.updateEnablements();

    // The selection is fixed once a transcription exists, and the "+" goes away with it. A menu
    // left open over that would be offering a choice that no longer applies.
    if (mProcessor.hasTranscription() && mInstrumentMenu.isVisible()) {
        _setInstrumentMenuOpen(false);
    }
}

void NeuralNoteMainView::valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property)
{
    if (property == NnId::PlayheadCenteredId) {
        const bool should_center = treeWhosePropertyHasChanged.getProperty(property);
        mTopBar.getFollowButton().setToggleState(should_center, sendNotification);
        mVisualizationPanel.getCombinedAudioMidiRegion().setCenterView(should_center);
    }

    if (property == NnId::SelectedInstrumentGroupsId) {
        // The transcribe button names how many instruments it is about to look for.
        mVisualizationPanel.updateEnablements();
    }
}

void NeuralNoteMainView::_buildSettingsMenu()
{
    mSettingsMenu = std::make_unique<PopupMenu>();
    mSettingsMenuItemsShouldBeTicked.clear();
    mSettingsMenuItemsShouldBeEnabled.clear();

    int item_id = 0;

    // Applies from the next transcription: the model is loaded per run.
    PopupMenu device_menu;
    String device_menu_name = "Compute device";

    if (const std::vector<msl::Device>* devices = ComputeDevices::tryGet()) {
        const auto is_selected = [devices](std::optional<std::size_t> inIndex) {
            return ComputeDevices::resolve(*devices, NnGlobalSettings::getComputeDevice()) == inIndex;
        };

        const auto add_device_item =
            [&](const String& inLabel, std::optional<std::size_t> inIndex, const ComputeDeviceChoice& inChoice) {
                auto device_item = PopupMenu::Item(inLabel);
                device_item.setID(++item_id);
                device_item.setEnabled(true);
                mSettingsMenuItemsShouldBeTicked.emplace_back(device_item.itemID,
                                                              [is_selected, inIndex] { return is_selected(inIndex); });
                device_item.setTicked(mSettingsMenuItemsShouldBeTicked.back().second());
                device_item.setAction([this, inChoice] {
                    NnGlobalSettings::setComputeDevice(inChoice);
                    _refreshSettingsMenu();
                });
                device_menu.addItem(device_item);
            };

        const String auto_label = "Auto (" + String(ComputeDevices::label(*devices, msl::autoDevice(*devices))) + ")";
        const auto selected = ComputeDevices::resolve(*devices, NnGlobalSettings::getComputeDevice());
        device_menu_name +=
            ": " + (selected.has_value() ? String(ComputeDevices::label(*devices, *selected)) : auto_label);

        add_device_item(auto_label, std::nullopt, {});
        device_menu.addSeparator();

        for (std::size_t i = 0; i < devices->size(); ++i) {
            add_device_item(ComputeDevices::label(*devices, i), i, ComputeDevices::choiceFor(*devices, i));
        }
    }

    else {
        auto detecting_item = PopupMenu::Item("Detecting devices...");
        detecting_item.setID(++item_id);
        detecting_item.setEnabled(false);
        mSettingsMenuItemsShouldBeEnabled.emplace_back(detecting_item.itemID, [] { return false; });
        device_menu.addItem(detecting_item);
    }

    mSettingsMenu->addSubMenu(device_menu_name, device_menu);
    mSettingsMenu->addSeparator();

    auto tooltip_visibility_item = PopupMenu::Item("Show tooltips");
    tooltip_visibility_item.setID(++item_id);
    tooltip_visibility_item.setEnabled(true);
    mSettingsMenuItemsShouldBeTicked.emplace_back(tooltip_visibility_item.itemID,
                                                  [] { return NnGlobalSettings::getTooltipsVisible(); });
    tooltip_visibility_item.setTicked(mSettingsMenuItemsShouldBeTicked.back().second());
    tooltip_visibility_item.setAction([this] {
        NnGlobalSettings::setTooltipsVisible(!NnGlobalSettings::getTooltipsVisible());
        _updateTooltipVisibility();
        _refreshSettingsMenu();
    });
    mSettingsMenu->addItem(tooltip_visibility_item);

    // The editor applies this factor, clamped to what the display can hold, and writes back the
    // one it used -- so a preset too large for the screen ends up unticked.
    PopupMenu scale_menu;

    for (const double scale: {0.5, 0.75, 1.0, 1.25, 1.5, 2.0}) {
        auto scale_item = PopupMenu::Item(String(roundToInt(scale * 100.0)) + "%");
        scale_item.setID(++item_id);
        scale_item.setEnabled(true);
        mSettingsMenuItemsShouldBeTicked.emplace_back(scale_item.itemID, [this, item_scale = scale] {
            auto* editor = findParentComponentOfClass<NeuralNoteEditor>();
            return editor != nullptr && std::abs(editor->getAppliedScale() - item_scale) < 0.005;
        });
        scale_item.setTicked(mSettingsMenuItemsShouldBeTicked.back().second());
        scale_item.setAction([this, item_scale = scale] {
            // applyScale stores what it settled on, by the same path a resize drag takes.
            if (auto* editor = findParentComponentOfClass<NeuralNoteEditor>()) {
                editor->applyScale(item_scale);
            }

            _refreshSettingsMenu();
        });
        scale_menu.addItem(scale_item);
    }

    mSettingsMenu->addSubMenu("Window size", scale_menu);

    auto reset_zoom_item = PopupMenu::Item("Reset piano roll zoom");
    reset_zoom_item.setID(++item_id);
    reset_zoom_item.setTicked(false);
    reset_zoom_item.setAction([this] {
        mProcessor.getValueTree().setProperty(NnId::ZoomLevelId, 1.0, nullptr);

        // Negative returns the vertical zoom to fitting itself to the transcription, which is what
        // it does before the slider is ever touched.
        mProcessor.getValueTree().setProperty(NnId::VerticalZoomId, -1.0, nullptr);
    });
    mSettingsMenu->addItem(reset_zoom_item);
    mSettingsMenu->addSeparator();

    // A dense mix can name more instruments than MIDI has channels; see MidiOverflowMode.
    PopupMenu overflow_menu;

    for (const auto& [mode, label]:
         {std::pair {MidiOverflowMode::ReuseChannels, "Reuse the last channels"},
          std::pair {MidiOverflowMode::DropExtraInstruments, "Drop the extra instruments"}}) {
        auto overflow_item = PopupMenu::Item(label);
        overflow_item.setID(++item_id);
        overflow_item.setEnabled(true);
        mSettingsMenuItemsShouldBeTicked.emplace_back(overflow_item.itemID, [this, item_mode = mode] {
            return static_cast<int>(mProcessor.getValueTree().getProperty(
                       NnId::MidiOverflowModeId, static_cast<int>(MidiOverflowMode::ReuseChannels)))
                   == static_cast<int>(item_mode);
        });
        overflow_item.setTicked(mSettingsMenuItemsShouldBeTicked.back().second());
        overflow_item.setAction([this, item_mode = mode] {
            mProcessor.getValueTree().setProperty(NnId::MidiOverflowModeId, static_cast<int>(item_mode), nullptr);
            _refreshSettingsMenu();
        });
        overflow_menu.addItem(overflow_item);
    }

    mSettingsMenu->addSubMenu("MIDI export on channel overflow", overflow_menu);

    auto check_updates_item = PopupMenu::Item("Check for updates");
    check_updates_item.setID(++item_id);
    check_updates_item.setEnabled(true);
    check_updates_item.setTicked(false);
    check_updates_item.setAction([this] { mUpdateCheck->checkForUpdate(true); });
    mSettingsMenu->addSeparator();
    mSettingsMenu->addItem(check_updates_item);
}

void NeuralNoteMainView::_refreshSettingsMenu()
{
    // An item with no predicate of its own falls back to inDefault: unticked, and enabled.
    const auto evaluate =
        [](const std::vector<std::pair<int, std::function<bool()>>>& inPredicates, int inItemId, bool inDefault) {
            for (const auto& [item_id, predicate]: inPredicates) {
                if (item_id == inItemId && predicate != nullptr) {
                    return predicate();
                }
            }

            return inDefault;
        };

    for (PopupMenu::MenuItemIterator iterator(*mSettingsMenu, true); iterator.next();) {
        auto& item = iterator.getItem();
        item.setTicked(evaluate(mSettingsMenuItemsShouldBeTicked, item.itemID, false));
        item.setEnabled(evaluate(mSettingsMenuItemsShouldBeEnabled, item.itemID, true));
    }
}

void NeuralNoteMainView::_updateTooltipVisibility()
{
    if (NnGlobalSettings::getTooltipsVisible()) {
        if (mTooltipWindow == nullptr) {
            mTooltipWindow = std::make_unique<TooltipWindow>(this, 800);
            mTooltipWindow->setOpaque(false);
        }
    } else {
        mTooltipWindow = nullptr;
    }
}
