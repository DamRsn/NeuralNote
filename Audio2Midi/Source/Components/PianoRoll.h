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
    float _timeToX(float time) const;

    float _noteToYStart(int note) const;

    const int mKeyboardWidth = 50;

    Keyboard mKeyboard;
    Audio2MidiAudioProcessor& mProcessor;
};

#endif // PianoRoll_h
