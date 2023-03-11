//
// Created by Damien Ronssin on 10.03.23.
//

#include "PianoRoll.h"

PianoRoll::PianoRoll(Audio2MidiAudioProcessor& processor,
                     Keyboard& keyboard,
                     double inNumPixelsPerSecond)
    : mProcessor(processor)
    , mKeyboard(keyboard)
    , mNumPixelsPerSecond(inNumPixelsPerSecond)
{
    mKeyboard.addChangeListener(this);

    mNoteGradient.addColour(0.0, juce::Colours::green);
    mNoteGradient.addColour(0.5, juce::Colours::blue);
    mNoteGradient.addColour(1.0, juce::Colours::red);
}

void PianoRoll::resized()
{
}

void PianoRoll::paint(Graphics& g)
{
    Rectangle<float> local_bounds = {
        0, 0, static_cast<float>(getWidth()), static_cast<float>(getHeight())};

    auto rect_width = static_cast<float>(getWidth());

    // Draw permanent lines
    for (int i = MIN_MIDI_NOTE; i <= MAX_MIDI_NOTE; i++)
    {
        if (mKeyboard.getRectangleForKey(i).intersects(local_bounds))
        {
            juce::Colour fill_colour =
                _isWhiteKey(i) ? juce::Colours::lightgrey : juce::Colours::darkgrey;

            fill_colour = fill_colour.withAlpha(0.2f);

            g.setColour(fill_colour);

            auto note_y_start_n_height = _getNoteHeightAndWidthPianoRoll(i);
            g.fillRect(0.0f,
                       note_y_start_n_height.first,
                       rect_width,
                       note_y_start_n_height.second);
        }
    }

    // Draw notes
    if (mProcessor.getState() == PopulatedAudioAndMidiRegions)
    {
        for (auto& note_event: mProcessor.getNoteEventVector())
        {
            auto note_y_start_n_height =
                _getNoteHeightAndWidthPianoRoll(note_event.pitch);
            auto start = static_cast<float>(note_event.start);
            auto end = static_cast<float>(note_event.end);

            if (note_y_start_n_height.first < 0
                || note_y_start_n_height.second >= static_cast<float>(getHeight()))
                continue;

            g.setColour(mNoteGradient.getColourAtPosition(note_event.amplitude));
            g.fillRect(_timeToX(start),
                       note_y_start_n_height.first,
                       _timeToX(end) - _timeToX(start),
                       note_y_start_n_height.second);

            g.setColour(juce::Colours::black);
            g.drawRect(_timeToX(start),
                       note_y_start_n_height.first,
                       _timeToX(end) - _timeToX(start),
                       note_y_start_n_height.second,
                       0.5);
        }
    }
}

void PianoRoll::changeListenerCallback(ChangeBroadcaster* source)
{
    if (source == &mKeyboard)
    {
        repaint();
    }
}

float PianoRoll::_timeToX(float inTime) const
{
    return inTime * static_cast<float>(mNumPixelsPerSecond);
}

std::pair<float, float> PianoRoll::_getNoteHeightAndWidthPianoRoll(int inNote) const
{
    jassert(inNote >= MIN_MIDI_NOTE && inNote <= MAX_MIDI_NOTE);

    if (inNote == MIN_MIDI_NOTE)
    {
        return {
            _noteBottomY(inNote + 1),
            _noteBottomY(inNote) - _noteBottomY(inNote + 1),
        };
    }
    else if (inNote == MAX_MIDI_NOTE)
    {
        return {_noteTopY(inNote), _noteTopY(inNote - 1) - _noteTopY(inNote)};
    }
    else
    {
        if (_isWhiteKey(inNote))
        {
            return {_noteBottomY(inNote + 1),
                    _noteTopY(inNote - 1) - _noteBottomY(inNote + 1)};
        }
        else
        {
            return {_noteTopY(inNote), mKeyboard.getBlackNoteWidth()};
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
