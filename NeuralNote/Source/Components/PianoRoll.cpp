//
// Created by Damien Ronssin on 10.03.23.
//

#include "PianoRoll.h"

PianoRoll::PianoRoll(NeuralNoteAudioProcessor& processor,
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

    g.setColour(WAVEFORM_BG_COLOR);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 4);

    if (mProcessor.getState() == PopulatedAudioAndMidiRegions)
    {
        // Draw horizontal note lines
        for (int i = MIN_MIDI_NOTE; i <= MAX_MIDI_NOTE; i++)
        {
            if (mKeyboard.getRectangleForKey(i).intersects(local_bounds))
            {
                juce::Colour fill_colour =
                    _isWhiteKey(i) ? juce::Colours::white : juce::Colours::black;

                fill_colour = fill_colour.withAlpha(0.2f);

                g.setColour(fill_colour);

                auto note_y_start_n_height = _getNoteHeightAndWidthPianoRoll(i);
                g.fillRect(0.0f,
                           note_y_start_n_height.first,
                           rect_width,
                           note_y_start_n_height.second);
            }
        }

        // Draw vertical lines if we have info on bpm, time signature ...
        if (mProcessor.canQuantize())
        {
            _drawBeatVerticalLines(g);
        }

        // Draw notes
        for (auto& note_event: mProcessor.getNoteEventVector())
        {
            auto note_y_start_n_height =
                _getNoteHeightAndWidthPianoRoll(note_event.pitch);
            auto note_y_start = note_y_start_n_height.first;
            auto note_height = note_y_start_n_height.second;
            auto start = static_cast<float>(note_event.startTime);
            auto end = static_cast<float>(note_event.endTime);

            if (note_y_start < 0 || note_height >= static_cast<float>(getHeight()))
                continue;

            g.setColour(mNoteGradient.getColourAtPosition(note_event.amplitude));
            g.fillRect(_timeToX(start),
                       note_y_start,
                       _timeToX(end) - _timeToX(start),
                       note_height);

            g.setColour(juce::Colours::black);
            g.drawRect(_timeToX(start),
                       note_y_start,
                       _timeToX(end) - _timeToX(start),
                       note_height,
                       0.5);

            // Draw pitch bend
            if (mProcessor.getCustomParameters()->pitchBendMode == SinglePitchBend)
            {
                const auto& bends = note_event.bends;
                if (!note_event.bends.empty())
                {
                    auto path_stroke_type = PathStrokeType(
                        1, juce::PathStrokeType::mitered, juce::PathStrokeType::butt);
                    Path p;
                    float y_ref_pb = note_y_start + note_height / 2.0f;

                    p.startNewSubPath(_timeToX(start), y_ref_pb);

                    for (size_t i = 0; i < bends.size(); i++)
                    {
                        p.lineTo(
                            _timeToX(float(
                                start + double(i) * FFT_HOP / BASIC_PITCH_SAMPLE_RATE)),
                            y_ref_pb - float(bends[i]) * note_height / 3.0f);
                    }
                    p.lineTo(_timeToX(float(note_event.endTime)), y_ref_pb);

                    g.setColour(WHITE_SOLID);
                    g.strokePath(p, path_stroke_type);

                    p.closeSubPath();

                    g.setColour(WHITE_TRANSPARENT.withAlpha(0.6f));
                    g.fillPath(p);
                }
            }
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

void PianoRoll::_drawBeatVerticalLines(Graphics& g)
{
    auto playhead_info = mProcessor.getPlayheadInfoOnRecordStart();
    jassert(playhead_info.hasValue());
    double beats_per_second = 60.0 / *playhead_info->getBpm();
    auto time_signature = *playhead_info->getTimeSignature();

    double last_bar_qn = *playhead_info->getPpqPositionOfLastBarStart();
    double start_time_qn = *playhead_info->getPpqPosition();

    double beat_increments = 4.0 / time_signature.denominator;
    double beat_number = 0;
    float beat_pixel =
        _qnToPixel(beat_number, last_bar_qn - start_time_qn, beats_per_second);

    auto width = static_cast<float>(getWidth());
    auto height = static_cast<float>(getHeight());

    g.setColour(WHITE_SOLID);

    while (beat_pixel < width)
    {
        if (beat_pixel >= 0)
        {
            float thickness =
                std::abs(fmod(beat_number, static_cast<double>(time_signature.numerator)))
                        < 1e-6
                    ? 1.0f
                    : 0.5f;
            g.drawLine(beat_pixel, 0, beat_pixel, height, thickness);
        }

        beat_number += beat_increments;
        beat_pixel =
            _qnToPixel(beat_number, last_bar_qn - start_time_qn, beats_per_second);
    }
}

float PianoRoll::_qnToPixel(double inQn, double inZeroQn, double inBeatsPerSecond) const
{
    return static_cast<float>((inQn + inZeroQn) * inBeatsPerSecond * mNumPixelsPerSecond);
}
