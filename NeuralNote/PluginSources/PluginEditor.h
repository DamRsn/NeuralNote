#pragma once

#include "NeuralNoteMainView.h"
#include "NnEditorConstrainer.h"
#include "PluginProcessor.h"

/**
 * The window, and the only place that knows the UI can be any size other than the authored one.
 *
 * The main view keeps its authored bounds at every scale and is mapped onto the window by an
 * affine transform, so nothing below re-lays out and no component knows what scale it is drawn
 * at. The factor lives in NnGlobalSettings; what a window applies is that factor clamped to the
 * display it opened on, and the clamped one is what is stored back.
 */
class NeuralNoteEditor : public juce::AudioProcessorEditor
{
public:
    explicit NeuralNoteEditor(NeuralNoteAudioProcessor&);

    ~NeuralNoteEditor() override;

    void paint(juce::Graphics&) override;

    void resized() override;

    void parentHierarchyChanged() override;

    NeuralNoteMainView* getMainView() const { return mMainView.get(); }

    /** Sizes the window from inScale, clamped to the display, and persists what it settled on. */
    void applyScale(double inScale);

    /** The factor actually in force, which a scale too large for the display will not match. */
    double getAppliedScale() const { return mScale; }

private:
    /** Sizes the window without persisting, for the paths that are not the user asking. */
    void _setScale(double inScale);

    /** Stores mScale globally, unless it is already what was stored. */
    void _persistScale();

    std::unique_ptr<NeuralNoteMainView> mMainView;

    NnEditorConstrainer mConstrainer;

    double mScale = 1.0;

    // What global.settings holds, so that opening and closing a window untouched writes nothing.
    double mPersistedScale = 1.0;
};
