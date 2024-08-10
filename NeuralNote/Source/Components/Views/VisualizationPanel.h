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
#include "NumericTextEditor.h"

class VisualizationPanel : public Component
{
public:
    explicit VisualizationPanel(NeuralNoteAudioProcessor* processor);

    ~VisualizationPanel() override = default;

    void resized() override;

    void paint(Graphics& g) override;

    void clear();

    void repaintPianoRoll();

    void setMidiFileDragComponentVisible();

    void mouseEnter(const MouseEvent& event) override;

    void mouseExit(const MouseEvent& event) override;

    Viewport& getAudioMidiViewport();

    CombinedAudioMidiRegion& getCombinedAudioMidiRegion();

    static constexpr int KEYBOARD_WIDTH = 50;

private:
    NeuralNoteAudioProcessor* mProcessor;
    Keyboard mKeyboard;
    Viewport mAudioMidiViewport;
    CombinedAudioMidiRegion mCombinedAudioMidiRegion;
    MidiFileDrag mMidiFileDrag;

    Slider mAudioGainSlider;
    std::unique_ptr<SliderParameterAttachment> mAudioGainSliderAttachment;

    Slider mMidiGainSlider;
    std::unique_ptr<SliderParameterAttachment> mMidiGainSliderAttachment;

    Rectangle<int> mAudioRegionBounds;
    Rectangle<int> mPianoRollBounds;

    std::unique_ptr<NumericTextEditor<double>> mFileTempo;
};
#endif // VisualizationPanel_h
