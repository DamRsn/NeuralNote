//
// Created by Damien Ronssin on 06.03.23.
//

#ifndef PluginMainView_h
#define PluginMainView_h

#include <JuceHeader.h>

#include "PluginProcessor.h"
#include "Knob.h"
#include "NoteOptionsView.h"
#include "TimeQuantizeOptionsView.h"
#include "TranscriptionOptionsView.h"
#include "VisualizationPanel.h"
#include "NeuralNoteLNF.h"
#include "NnId.h"
#include "UpdateCheck.h"

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

    bool keyPressed(const KeyPress& key) override;

private:
    void updateEnablements();

    void valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property) override;

    void _updateSettingsMenuTicks();

    void _updateTooltipVisibility();

    NeuralNoteAudioProcessor& mProcessor;
    NeuralNoteLNF mLNF;

    State mPrevState = EmptyAudioAndMidiRegions;

    VisualizationPanel mVisualizationPanel;
    TranscriptionOptionsView mTranscriptionOptions;
    NoteOptionsView mNoteOptions;
    TimeQuantizeOptionsView mQuantizePanel;

    std::unique_ptr<DrawableButton> mMuteButton;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> mMuteButtonAttachment;

    std::unique_ptr<DrawableButton> mRecordButton;
    std::unique_ptr<DrawableButton> mClearButton;

    std::unique_ptr<DrawableButton> mBackButton;
    std::unique_ptr<DrawableButton> mPlayPauseButton;
    std::unique_ptr<DrawableButton> mCenterButton;
    std::unique_ptr<DrawableButton> mSettingsButton;

    std::unique_ptr<TooltipWindow> mTooltipWindow;

    class PopupMenuLookAndFeel : public LookAndFeel_V4
    {
        Font getPopupMenuFont() override { return UIDefines::LABEL_FONT(); }
    };

    std::unique_ptr<PopupMenuLookAndFeel> mPopupMenuLookAndFeel;
    // Define the settings menu after the look and feel, so it is destroyed first
    std::unique_ptr<PopupMenu> mSettingsMenu;

    std::vector<std::pair<int, std::function<bool()>>> mSettingsMenuItemsShouldBeTicked;

    std::unique_ptr<Knob> mMinNoteSlider;
    std::unique_ptr<Knob> mMaxNoteSlider;

    std::unique_ptr<ComboBox> mKey; // C, C#, D, D# ...
    std::unique_ptr<ComboBox> mMode; // Major, Minor, Chromatic

    Image mBackgroundImage;

    int mNumCallbacksStuckInProcessingState = 0;

    std::unique_ptr<UpdateCheck> mUpdateCheck;
};

#endif // PluginMainView_h
