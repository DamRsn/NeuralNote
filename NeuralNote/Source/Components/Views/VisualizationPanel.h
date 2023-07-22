//
// Created by Damien Ronssin on 11.03.23.
//

#ifndef VisualizationPanel_h
#define VisualizationPanel_h

#include <JuceHeader.h>

#include "CombinedAudioMidiRegion.h"
#include "Keyboard.h"
#include "MidiFileDrag.h"
#include "PluginProcessor.h"
#include "VisualizationPanel.h"

class VisualizationPanel : public juce::Component,
                           public juce::Timer // To update play/pause button state from processor
{
public:
    explicit VisualizationPanel(NeuralNoteAudioProcessor& processor);

    void resized() override;

    void paint(Graphics& g) override;

    void timerCallback() override;

    void clear();

    void repaintPianoRoll();

    void setMidiFileDragComponentVisible();

    static constexpr int KEYBOARD_WIDTH = 50;

private:
    NeuralNoteAudioProcessor& mProcessor;
    Keyboard mKeyboard;
    juce::Viewport mAudioMidiViewport;
    CombinedAudioMidiRegion mCombinedAudioMidiRegion;
    MidiFileDrag mMidiFileDrag;

    juce::TextButton mPlayPauseButton;
    juce::TextButton mResetButton;

    std::unique_ptr<juce::TextEditor> mFileTempo;
};

#endif // VisualizationPanel_h
