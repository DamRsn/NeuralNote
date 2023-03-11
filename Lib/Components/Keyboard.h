//
// Created by Damien Ronssin on 10.03.23.
//

#ifndef Keyboard_h
#define Keyboard_h

#include <JuceHeader.h>

#include "Constants.h"

class Keyboard : public KeyboardComponentBase
{
public:
    Keyboard();

private:
    void drawKeyboardBackground(Graphics& g, Rectangle<float> area) override;

    void drawWhiteKey(int midiNoteNumber, Graphics& g, Rectangle<float> area) override;

    void drawBlackKey(int midiNoteNumber, Graphics& g, Rectangle<float> area) override;
};

#endif // Keyboard_h
