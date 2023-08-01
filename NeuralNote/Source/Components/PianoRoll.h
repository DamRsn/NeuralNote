//
// Created by Damien Ronssin on 10.03.23.
//

#ifndef PianoRoll_h
#define PianoRoll_h

#include <JuceHeader.h>

#include "BasicPitchConstants.h"
#include "Keyboard.h"
#include "PluginProcessor.h"
#include "UIDefines.h"
#include "Playhead.h"

class VisualizationPanel;

class PianoRoll
    : public juce::Component
    , public juce::ChangeListener
{
public:
    PianoRoll(NeuralNoteAudioProcessor& inProcessor,
              Keyboard& keyboard,
              double inNumPixelsPerSecond,
              VisualizationPanel* inVisualizationPanel);

    void resized() override;

    void paint(Graphics& g) override;

    void changeListenerCallback(ChangeBroadcaster* source) override;

    void mouseDown(const MouseEvent& event) override;

    void mouseEnter(const juce::MouseEvent& event) override;

    void mouseExit(const juce::MouseEvent& event) override;

private:
    float _timeToPixel(float inTime) const;

    float _pixelToTime(float inPixel) const;

    /**
     * Compute rectangle y start and height to draw inNote on piano roll
     * @param inNote Midi note
     * @return Rectangle y start and height
     */
    std::pair<float, float> _getNoteHeightAndWidthPianoRoll(int inNote) const;

    /**
     * Top limit of note on keyboard.
     * @param inNote
     * @return
     */
    float _noteTopY(int inNote) const;

    /** Bottom limit of note on keyboard
     *
     * @param inNote
     * @return
     */
    float _noteBottomY(int inNote) const;

    static bool _isWhiteKey(int inNote);

    float _getNoteWidth(int inNote) const;

    void _drawBeatVerticalLines(Graphics& g);

    float _qnToPixel(double inQn, double inZeroQn, double inBeatsPerSecond) const;

    const double mNumPixelsPerSecond;

    juce::ColourGradient mNoteGradient;

    Keyboard& mKeyboard;
    NeuralNoteAudioProcessor& mProcessor;
    VisualizationPanel* mVisualizationPanel;

    Playhead mPlayhead;
};

#endif // PianoRoll_h
