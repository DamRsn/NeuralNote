//
// Created by Damien Ronssin on 10.03.23.
//

#ifndef PianoRoll_h
#define PianoRoll_h

#include <JuceHeader.h>

#include "Constants.h"
#include "Keyboard.h"
#include "PluginProcessor.h"
#include "UIDefines.h"

class PianoRoll
    : public juce::Component
    , public juce::ChangeListener
{
public:
    explicit PianoRoll(NeuralNoteAudioProcessor& processor,
                       Keyboard& keyboard,
                       double inNumPixelsPerSecond);

    void resized() override;

    void paint(Graphics& g) override;

    void changeListenerCallback(ChangeBroadcaster* source) override;

private:
    float _timeToX(float inTime) const;

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
};

#endif // PianoRoll_h
