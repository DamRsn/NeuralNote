//
// Created by Damien Ronssin on 06.03.23.
//

#ifndef PluginMainView_h
#define PluginMainView_h

#include <JuceHeader.h>

#include "InstrumentMenu.h"
#include "NnId.h"
#include "PluginProcessor.h"
#include "Sidebar.h"
#include "TopBar.h"
#include "UpdateCheck.h"
#include "VisualizationPanel.h"

class NeuralNoteMainView
    : public Component
    , public Timer
    , public ValueTree::Listener
{
public:
    explicit NeuralNoteMainView(NeuralNoteAudioProcessor& processor);

    ~NeuralNoteMainView() override;

    void resized() override;

    void paint(Graphics& g) override;

    void timerCallback() override;

    void repaintPianoRoll();

    /**
     * Resets the view to its empty state. Called by the processor from every path that clears,
     * so it must stay idempotent.
     */
    void clear();

    bool keyPressed(const KeyPress& key) override;

private:
    void updateEnablements();

    void valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property) override;

    void _buildSettingsMenu();

    /** Re-evaluates every tick and enablement predicate. The menu is built once, so this is what
        keeps it in step with settings another instance may have changed. */
    void _refreshSettingsMenu();

    void _updateTooltipVisibility();

    /** Shows or hides the instrument picker, and lights the "+" to match. */
    void _setInstrumentMenuOpen(bool inIsOpen);

    NeuralNoteAudioProcessor& mProcessor;

    State mPrevState = EmptyAudioAndMidiRegions;

    TopBar mTopBar;
    Sidebar mSidebar;
    VisualizationPanel mVisualizationPanel;

    std::unique_ptr<TooltipWindow> mTooltipWindow;

    // A child of the main view rather than of the sidebar: its scrim covers the whole editor, and
    // its shadow falls outside the sidebar's bounds.
    InstrumentMenu mInstrumentMenu;

    std::unique_ptr<PopupMenu> mSettingsMenu;

    std::vector<std::pair<int, std::function<bool()>>> mSettingsMenuItemsShouldBeTicked;

    // Items absent from this one stay enabled.
    std::vector<std::pair<int, std::function<bool()>>> mSettingsMenuItemsShouldBeEnabled;

    std::unique_ptr<UpdateCheck> mUpdateCheck;
};

#endif // PluginMainView_h
