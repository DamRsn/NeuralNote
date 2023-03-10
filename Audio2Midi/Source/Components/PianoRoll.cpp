//
// Created by Damien Ronssin on 10.03.23.
//

#include "PianoRoll.h"

PianoRoll::PianoRoll(Audio2MidiAudioProcessor& processor)
    : mProcessor(processor)
{
    addAndMakeVisible(mKeyboard);
}

void PianoRoll::resized()
{
    mKeyboard.setBounds(0, 0, mKeyboardWidth, getHeight());
}

void PianoRoll::paint(Graphics& g)
{
    g.setColour(juce::Colours::pink);

    if (mProcessor.getState() == PopulatedAudioAndMidiRegions)
    {
        float key_width = mKeyboard.getKeyWidth();

        for (auto& note_event: mProcessor.getNoteEventVector())
        {
            auto note_y_start = _noteToYStart(note_event.pitch);
            auto start = static_cast<float>(note_event.start);
            auto end = static_cast<float>(note_event.end);

            if (note_y_start < 0 || note_y_start >= static_cast<float>(getHeight()))
                continue;

            g.fillRect(_timeToX(start),
                       note_y_start,
                       _timeToX(end) - _timeToX(start),
                       key_width);
        }
    }
}

float PianoRoll::_timeToX(float time) const
{
    return time / 10.0f * static_cast<float>(getWidth() - mKeyboardWidth)
           + static_cast<float>(mKeyboardWidth);
}

float PianoRoll::_noteToYStart(int note) const
{
    return static_cast<float>(getHeight()) - mKeyboard.getKeyStartPosition(note)
           - mKeyboard.getKeyWidth();
}
