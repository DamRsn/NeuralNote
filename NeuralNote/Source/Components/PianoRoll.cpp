//
// Created by Damien Ronssin on 10.03.23.
//

#include "PianoRoll.h"

PianoRoll::PianoRoll(NeuralNoteAudioProcessor* inProcessor, Keyboard& keyboard, double inBaseNumPixelsPerSecond)
    : mBaseNumPixelsPerSecond(inBaseNumPixelsPerSecond)
      , mKeyboard(keyboard)
      , mProcessor(inProcessor)
      , mPlayhead(inProcessor, inBaseNumPixelsPerSecond)
{
    mKeyboard.addChangeListener(this);

    mNoteGradient.addColour(0.0, Colours::green);
    mNoteGradient.addColour(0.5, Colours::blue);
    mNoteGradient.addColour(1.0, Colours::red);

    addAndMakeVisible(mPlayhead);

    mProcessor->addListenerToStateValueTree(this);
}

PianoRoll::~PianoRoll()
{
    mProcessor->removeListenerFromStateValueTree(this);
}

void PianoRoll::resized()
{
    mPlayhead.setSize(getWidth(), getHeight());
}

void PianoRoll::setZoomLevel(double inZoomLevel)
{
    mZoomLevel = inZoomLevel;
    mPlayhead.setZoomLevel(inZoomLevel);
    repaint();
}

void PianoRoll::paint(Graphics& g)
{
    Rectangle<float> local_bounds = {0, 0, static_cast<float>(getWidth()), static_cast<float>(getHeight())};

    auto rect_width = static_cast<float>(getWidth());

    g.setColour(WAVEFORM_BG_COLOR);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 4);

    if (mProcessor->getState() == PopulatedAudioAndMidiRegions) {
        // Draw horizontal note lines
        for (int i = MIN_MIDI_NOTE; i <= MAX_MIDI_NOTE; i++) {
            if (mKeyboard.getRectangleForKey(i).intersects(local_bounds)) {
                Colour fill_colour = _isWhiteKey(i) ? Colours::white : Colours::black;

                fill_colour = fill_colour.withAlpha(0.2f);

                g.setColour(fill_colour);

                auto [note_y_start, note_height] = _getNoteHeightAndWidthPianoRoll(i);
                g.fillRect(0.0f, note_y_start, rect_width, note_height);
            }
        }

        // Draw vertical lines if we have info on bpm, time signature ...
        if (mProcessor->getParameterValue(ParameterHelpers::EnableTimeQuantizationId) > 0.5f) {
            _drawBeatVerticalLines(g);
        }

        // Draw notes
        for (auto& note_event: mProcessor->getTranscriptionManager()->getNoteEventVector()) {
            auto [note_y_start, note_height] = _getNoteHeightAndWidthPianoRoll(note_event.pitch);
            auto start = static_cast<float>(note_event.startTime);
            auto end = static_cast<float>(note_event.endTime);

            if (note_y_start < 0 || note_height >= static_cast<float>(getHeight()))
                continue;

            g.setColour(mNoteGradient.getColourAtPosition(note_event.amplitude));
            g.fillRect(_timeToPixel(start), note_y_start, _timeToPixel(end) - _timeToPixel(start), note_height);

            g.setColour(Colours::black);
            g.drawRect(_timeToPixel(start), note_y_start, _timeToPixel(end) - _timeToPixel(start), note_height, 0.5);

            // Draw pitch bend
            if (static_cast<PitchBendModes>(
                    static_cast<int>(std::round(mProcessor->getParameterValue(ParameterHelpers::PitchBendModeId))))
                == SinglePitchBend) {
                const auto& bends = note_event.bends;

                if (!note_event.bends.empty()) {
                    auto path_stroke_type = PathStrokeType(1, PathStrokeType::mitered, PathStrokeType::butt);
                    Path p;
                    float y_ref_pb = note_y_start + note_height / 2.0f;

                    p.startNewSubPath(_timeToPixel(start), y_ref_pb);

                    for (size_t i = 0; i < bends.size(); i++) {
                        p.lineTo(_timeToPixel(static_cast<float>(
                                     start + static_cast<double>(i) * FFT_HOP / BASIC_PITCH_SAMPLE_RATE)),
                                 y_ref_pb - static_cast<float>(bends[i]) * note_height / 3.0f);
                    }
                    p.lineTo(_timeToPixel(static_cast<float>(note_event.endTime)), y_ref_pb);

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
    if (source == &mKeyboard) {
        repaint();
    }
}

void PianoRoll::mouseDown(const MouseEvent& event)
{
    mPlayhead.setPlayheadTime(_pixelToTime(static_cast<float>(event.x)));
}

void PianoRoll::valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property)
{
    if (property == NnId::TempoId || property == NnId::TimeSignatureNumeratorId
        || property == NnId::TimeSignatureDenominatorId || property == NnId::TimeQuantizeRefLastBarQnId
        || property == NnId::TimeQuantizeRefPosSec || property == NnId::TimeQuantizeRefPosQnId) {
        repaint();
    }
}

float PianoRoll::_timeToPixel(float inTime) const
{
    return inTime * static_cast<float>(mBaseNumPixelsPerSecond * mZoomLevel);
}

float PianoRoll::_pixelToTime(float inPixel) const
{
    return inPixel / static_cast<float>(mBaseNumPixelsPerSecond * mZoomLevel);
}

std::pair<float, float> PianoRoll::_getNoteHeightAndWidthPianoRoll(int inNote) const
{
    jassert(inNote >= MIN_MIDI_NOTE && inNote <= MAX_MIDI_NOTE);

    if (inNote == MIN_MIDI_NOTE) {
        return {
            _noteBottomY(inNote + 1),
            _noteBottomY(inNote) - _noteBottomY(inNote + 1),
        };
    } else if (inNote == MAX_MIDI_NOTE) {
        return {_noteTopY(inNote), _noteTopY(inNote - 1) - _noteTopY(inNote)};
    } else {
        if (_isWhiteKey(inNote)) {
            return {_noteBottomY(inNote + 1), _noteTopY(inNote - 1) - _noteBottomY(inNote + 1)};
        } else {
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
    return (note == 0 || note == 2 || note == 4 || note == 5 || note == 7 || note == 9 || note == 11);
}

float PianoRoll::_getNoteWidth(int inNote) const
{
    return _isWhiteKey(inNote) ? mKeyboard.getKeyWidth() : mKeyboard.getBlackNoteWidth();
}

void PianoRoll::_drawBeatVerticalLines(Graphics& g) const
{
    const auto tq_info = mProcessor->getTranscriptionManager()->getTimeQuantizeOptions().getTimeQuantizeInfo();
    const double seconds_per_beat = 60.0 / tq_info.bpm;

    const double start_bar_qn = tq_info.getStartLastBarQn();
    const double start_time_qn = tq_info.getStartQn();
    const double offset_bar_start = start_time_qn - start_bar_qn;

    jassert(start_bar_qn <= start_time_qn);

    const double beat_increments = 4.0 / tq_info.timeSignatureDenom;

    // Beat number in quarter notes
    double beat_pos_qn = 0;
    double beat_pixel = _beatPosQnToPixel(beat_pos_qn, offset_bar_start, seconds_per_beat);

    const auto width = static_cast<float>(getWidth());
    const auto height = static_cast<float>(getHeight());

    g.setColour(WHITE_SOLID);

    while (beat_pixel < width) {
        if (beat_pixel >= 0) {
            const float thickness =
                std::abs(std::fmod(beat_pos_qn, static_cast<double>(tq_info.timeSignatureNum))) < 1e-6 ? 1.0f : 0.5f;
            g.fillRect(Rectangle<float>(static_cast<float>(beat_pixel) - thickness / 2.0f, 0, thickness, height));
        }

        beat_pos_qn += beat_increments;
        beat_pixel = _beatPosQnToPixel(beat_pos_qn, offset_bar_start, seconds_per_beat);
    }
}

double PianoRoll::_beatPosQnToPixel(double inPosQn, double inOffsetBarStart, double inSecondsPerBeat) const
{
    return static_cast<float>((inPosQn - inOffsetBarStart) * inSecondsPerBeat * mBaseNumPixelsPerSecond * mZoomLevel);
}