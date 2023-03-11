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
    mKeyboard.setBounds(0, 0, KEYBOARD_WIDTH, getHeight());
}

void PianoRoll::paint(Graphics& g)
{
    Rectangle<float> local_bounds = {
        0, 0, static_cast<float>(getWidth()), static_cast<float>(getHeight())};

    auto rect_left = static_cast<float>(KEYBOARD_WIDTH);
    auto rect_width = static_cast<float>(getWidth()) - rect_left;

    for (int i = 21; i < 109; i++)
    {
        if (mKeyboard.getRectangleForKey(i).intersects(local_bounds))
        {
            juce::Colour fill_colour =
                _isWhiteKey(i) ? juce::Colours::lightgrey : juce::Colours::darkgrey;

            fill_colour = fill_colour.withAlpha(0.2f);

            g.setColour(fill_colour);

            auto y_range = _noteToYRange(i);
            g.fillRect(
                rect_left, y_range.second, rect_width, y_range.first - y_range.second);
        }
    }

    g.setColour(juce::Colours::pink);

    if (mProcessor.getState() == PopulatedAudioAndMidiRegions)
    {
        for (auto& note_event: mProcessor.getNoteEventVector())
        {
            auto note_y_range = _noteToYRange(note_event.pitch);
            auto start = static_cast<float>(note_event.start);
            auto end = static_cast<float>(note_event.end);

            if (note_y_range.first < 0
                || note_y_range.second >= static_cast<float>(getHeight()))
                continue;

            g.fillRect(_timeToX(start),
                       note_y_range.second,
                       _timeToX(end) - _timeToX(start),
                       note_y_range.first - note_y_range.second);
        }
    }
}

float PianoRoll::_timeToX(float inTime) const
{
    return inTime / 10.0f * static_cast<float>(getWidth() - KEYBOARD_WIDTH)
           + static_cast<float>(KEYBOARD_WIDTH);
}

std::pair<float, float> PianoRoll::_noteToYRange(int inNote) const
{
    jassert(inNote >= MIN_MIDI_NOTE && inNote <= MAX_MIDI_NOTE);

    if (inNote == MIN_MIDI_NOTE)
    {
        return {_noteBottomY(inNote), _noteBottomY(inNote + 1)};
    }
    else if (inNote == MAX_MIDI_NOTE)
    {
        return {_noteTopY(inNote - 1), _noteTopY(inNote)};
    }
    else
    {
        if (_isWhiteKey(inNote))
        {
            return {_noteTopY(inNote - 1), _noteBottomY(inNote + 1)};
        }
        else
        {
            return {_noteBottomY(inNote), _noteTopY(inNote)};
        }
    }
}

float PianoRoll::_noteTopY(int inNote) const
{
    return _noteBottomY(inNote) - _getNoteWidth(inNote);
}

float PianoRoll::_noteBottomY(int inNote) const
{
    return static_cast<float>(getHeight()) - mKeyboard.getKeyStartPosition(inNote);
}

bool PianoRoll::_isWhiteKey(int inNote)
{
    int note = inNote % 12;
    return (note == 0 || note == 2 || note == 4 || note == 5 || note == 7 || note == 9
            || note == 11);
}

float PianoRoll::_getNoteWidth(int inNote) const
{
    return _isWhiteKey(inNote) ? mKeyboard.getKeyWidth() : mKeyboard.getBlackNoteWidth();
}
