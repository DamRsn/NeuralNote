//
// Created by Damien Ronssin on 10.03.23.
//

#ifndef Keyboard_h
#define Keyboard_h

#include <JuceHeader.h>

#include "PianoRollRange.h"
#include "TranscriptionConstants.h"

/**
 * The key column left of the piano roll, and the roll's vertical geometry with it: every lane the
 * roll draws is measured off this component, so restricting the range here is what restricts the
 * roll.
 */
class Keyboard : public KeyboardComponentBase
{
public:
    Keyboard();

    /** Narrows the keyboard to the octaves worth showing. See PianoRollRange for what those are. */
    void setDisplayedRange(const PianoRollRange::DisplayRange& inRange);

    PianoRollRange::DisplayRange getDisplayedRange() const;

    /**
     * Sets one white key's height, which is the piano roll's vertical zoom: every lane the roll
     * draws is measured off this component, so this is the only place the zoom has to land.
     */
    void setWhiteKeyHeight(float inHeightPx);

    /** Holds the keys back while the roll beside them has nothing in it. */
    void setDimmed(bool inIsDimmed);

    /**
     * Scrolls the visible window by a wheel gesture the roll received rather than this component.
     * Both halves of the pitch axis have to move together, and JUCE's own handler is what gives
     * the scroll sub-key precision.
     */
    void scrollByWheel(const MouseEvent& inEvent, const MouseWheelDetails& inWheel);

private:
    void drawKeyboardBackground(Graphics& g, Rectangle<float> area) override;

    void drawWhiteKey(int midiNoteNumber, Graphics& g, Rectangle<float> area) override;

    void drawBlackKey(int midiNoteNumber, Graphics& g, Rectangle<float> area) override;

    /** Nothing: the design has no scroll arrows over the keys, and the wheel is how the view moves. */
    void drawUpDownButton(
        Graphics& g, int w, int h, bool isMouseOver, bool isButtonPressed, bool movesOctavesUp) override;

    /** How many semitones fit in the column at the current key height. */
    int _visibleSemitones() const;

    Colour _keyColour(Colour inColour) const;

    bool mIsDimmed = false;
};

#endif // Keyboard_h
