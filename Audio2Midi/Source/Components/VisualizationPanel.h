//
// Created by Damien Ronssin on 11.03.23.
//

#ifndef VisualizationPanel_h
#define VisualizationPanel_h

#include <JuceHeader.h>

#include "CombinedAudioMidiRegion.h"
#include "Keyboard.h"
#include "PluginProcessor.h"
#include "VisualizationPanel.h"

class VisualizationPanel : public juce::Component

{
public:
    explicit VisualizationPanel(Audio2MidiAudioProcessor& processor);

    void resized() override;

    void paint(Graphics& g) override;

    void startTimerHzAudioThumbnail(int inFreqHz);

    void stopTimerAudioThumbnail();

    static constexpr int KEYBOARD_WIDTH = 50;

private:
    Keyboard mKeyboard;

    juce::Viewport mAudioMidiViewport;
    CombinedAudioMidiRegion mCombinedAudioMidiRegion;
};

#endif // VisualizationPanel_h
