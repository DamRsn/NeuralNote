//
// Created by Damien Ronssin on 10.03.23.
//

#ifndef PianoRoll_h
#define PianoRoll_h

#include <JuceHeader.h>

#include "Keyboard.h"
#include "PluginProcessor.h"

class PianoRoll : public juce::Component
{
public:
    PianoRoll(Audio2MidiAudioProcessor& processor);

    void resized() override;

    void paint(Graphics& g) override;

private:
    float _timeToX(float inTime) const;

    /**
     * Returns upper and lower y coordinates to paint midi inNote rectangle.
     * @param inNote midi note
     * @return upper and lower limits of inNote on y axis (upper < lower)
     */
    std::pair<float, float> _noteToYRange(int inNote) const;

    /**
     * Top limit of note on piano.
     * @param inNote
     * @return
     */
    float _noteTopY(int inNote) const;

    /** Bottom limit of note on piano
     *
     * @param inNote
     * @return
     */
    float _noteBottomY(int inNote) const;

    static bool _isWhiteKey(int inNote);

    float _getNoteWidth(int inNote) const;

    const int mKeyboardWidth = 50;

    Keyboard mKeyboard;
    Audio2MidiAudioProcessor& mProcessor;
};

#endif // PianoRoll_h
