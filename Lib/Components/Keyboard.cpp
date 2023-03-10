//
// Created by Damien Ronssin on 10.03.23.
//

#include "Keyboard.h"

Keyboard::Keyboard()
    : KeyboardComponentBase(
        juce::KeyboardComponentBase::Orientation::verticalKeyboardFacingRight)
{
    setAvailableRange(21, 108);

    setBlackNoteWidthProportion(0.8f);
    setKeyWidth(14);
}

void Keyboard::drawKeyboardBackground(Graphics& g, Rectangle<float> area)
{
    g.setColour(juce::Colours::lightgrey);
    g.fillRect(area);
}

void Keyboard::drawWhiteKey(int midiNoteNumber, Graphics& g, Rectangle<float> area)
{
    g.setColour(juce::Colours::white);
    g.fillRect(area);

    g.setColour(juce::Colours::darkgrey);
    g.drawRect(area, 0.5);

    if (midiNoteNumber % 12 == 0)
    {
        int octave_number = midiNoteNumber / 12 - 2;
        g.setFont(getKeyWidth() - 3);
        g.drawText("C" + std::to_string(octave_number),
                   area.withTrimmedLeft(getBlackNoteLength()),
                   juce::Justification::centred);
    }
}

void Keyboard::drawBlackKey(int midiNoteNumber, Graphics& g, Rectangle<float> area)
{
    g.setColour(juce::Colours::black);
    g.fillRect(area);
    g.setColour(juce::Colours::darkgrey);
    g.drawRect(area, 0.5);
}
