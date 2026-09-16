//
// Created by Damien Ronssin on 11.03.23.
//

#ifndef VisualizationPanel_h
#define VisualizationPanel_h

#include <JuceHeader.h>

#include "CombinedAudioMidiRegion.h"
#include "Keyboard.h"
#include "ModelDownloadPanel.h"
#include "NnFlatButton.h"
#include "NnToolbar.h"
#include "PluginProcessor.h"
#include "StatusBar.h"
#include "TimelineGutter.h"

/**
 * Everything to the right of the sidebar: the toolbar, the waveform, the time ruler, the piano roll
 * and the status bar.
 *
 * The waveform, ruler and piano roll live inside one horizontally scrolling viewport, and the fixed
 * 46 px column to its left holds the amplitude scale and the keyboard. That split is what keeps the
 * three time axes aligned while the whole timeline zooms and scrolls as one.
 */
class VisualizationPanel
    : public Component
    , private juce::ChangeListener
    , private juce::ValueTree::Listener
{
public:
    explicit VisualizationPanel(NeuralNoteAudioProcessor* processor);

    ~VisualizationPanel() override;

    void resized() override;

    void clear();

    void repaintPianoRoll();

    /** Re-derives which toolbar controls are live from the processor's state. */
    void updateEnablements();

    /** Opens or closes the model panel. While there is no checkpoint on an idle roll it stays up regardless. */
    void setModelPanelOpen(bool inOpen);

    bool isModelPanelVisible() const;

    Viewport& getAudioMidiViewport();

    CombinedAudioMidiRegion& getCombinedAudioMidiRegion();

private:
    /** Follows the instrument mixer, which reports the transcription's pitch range as it grows. */
    void changeListenerCallback(juce::ChangeBroadcaster* inSource) override;

    /**
     * Centres the transcribe button on the empty piano roll and says whether it belongs there. With no
     * checkpoint installed, the model download panel takes its place.
     */
    void _layOutTranscribeButton();

    /**
     * Narrows the keyboard, and with it the piano roll, to the octaves worth showing.
     *
     * @param inMayShrink Whether the range is allowed to close in. False while a transcription is
     *        streaming, where every chunk can widen it and shrinking between chunks would make the
     *        view jump under the user. True when a run starts and when it finishes, which is where
     *        the range settles on what the finished transcription actually needs.
     */
    void _updateNoteRange(bool inMayShrink);

    void valueTreePropertyChanged(juce::ValueTree& inTree, const juce::Identifier& inProperty) override;

    /**
     * Pushes the stored vertical zoom into the keyboard and the slider, picking the zoom that fits
     * the transcription when nothing has been stored. Does not write back: an automatic choice that
     * saved itself would stop being automatic the first time the window was laid out.
     */
    void _applyVerticalZoom();

    NeuralNoteAudioProcessor* mProcessor;

    NnToolbar mToolbar;
    TimelineGutter mGutter;
    Keyboard mKeyboard;
    Viewport mAudioMidiViewport;
    CombinedAudioMidiRegion mCombinedAudioMidiRegion;
    StatusBar mStatusBar;

    // A child of this panel rather than of the piano roll, which lives inside a horizontally
    // scrolling viewport and would carry the button off screen with it.
    NnFlatButton mTranscribeButton {"Transcribe"};

    // Same parent and the same place as the transcribe button, which it stands in for.
    ModelDownloadPanel mModelDownloadPanel;

    // Whether the top bar's model button asked for the panel, as opposed to its being required.
    bool mModelPanelOpen = false;

    // What _updateNoteRange last settled on, so a state change can tell a widening apart from a
    // fresh start without asking the keyboard.
    State mPrevStateForRange = EmptyAudioAndMidiRegions;
};
#endif // VisualizationPanel_h
