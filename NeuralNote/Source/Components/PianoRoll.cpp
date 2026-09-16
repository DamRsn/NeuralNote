//
// Created by Damien Ronssin on 10.03.23.
//

#include "PianoRoll.h"

#include "NnLook.h"

namespace
{
constexpr float NOTE_CORNER = 2.0f;
constexpr float ONSET_EDGE_WIDTH = 2.0f;

// No shading by velocity: the model emits none, so every audible note is drawn at full colour.

/** How far a muted instrument's notes fall back. */
constexpr float MUTED_NOTE_ALPHA = 0.16f;

// A drum hit carries the library's 10 ms floor rather than a measured release, so at its real
// length it draws as a sliver. Widened for drawing only -- the note vector keeps the offset.
constexpr double DRUM_MIN_DRAWN_SECONDS = 0.1;

/** How far the lane stripes fall back while there is nothing drawn on them. */
constexpr float EMPTY_LANE_ALPHA = 0.55f;
} // namespace

PianoRoll::PianoRoll(NeuralNoteAudioProcessor* inProcessor, Keyboard& keyboard, double inBaseNumPixelsPerSecond)
    : mBaseNumPixelsPerSecond(inBaseNumPixelsPerSecond)
    , mKeyboard(keyboard)
    , mProcessor(inProcessor)
    , mPlayhead(inProcessor, inBaseNumPixelsPerSecond)
    , mVBlankAttachment(this, [this]() { _onVBlankCallback(); })
{
    mKeyboard.addChangeListener(this);
    mProcessor->getInstrumentMixer()->addChangeListener(this);

    mPlayhead.setDrawTriangle(false);
    addAndMakeVisible(mPlayhead);
}

PianoRoll::~PianoRoll()
{
    mProcessor->getInstrumentMixer()->removeChangeListener(this);
    mKeyboard.removeChangeListener(this);
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

void PianoRoll::updateEnablements()
{
    // With audio loaded and nothing transcribed, the roll is empty and the transcribe button sits
    // in the middle of it. A playhead sweeping across that button is noise over the one thing
    // there is to do here, so it is held back until there is a transcription to follow.
    const State state = mProcessor->getState();

    mPlayhead.setVisible(state != AudioLoaded && state != EmptyAudioAndMidiRegions);
    repaint();
}

void PianoRoll::paint(Graphics& g)
{
    g.fillAll(nn::colours::bgRoot);

    if (!mProcessor->canPlay()) {
        return;
    }

    _drawLanes(g);
    _drawNotes(g);

    const auto playhead_x = mPlayhead.isVisible() ? static_cast<int>(std::round(mPlayhead.getPlayheadX())) : 0;

    if (playhead_x > 0) {
        g.setColour(nn::colours::accentWashRoll());
        g.fillRect(0, 0, playhead_x, getHeight());
    }

    _drawTranscriptionFrontier(g);
}

void PianoRoll::_drawLanes(Graphics& g) const
{
    const Rectangle<float> local_bounds = getLocalBounds().toFloat();
    const auto width = static_cast<float>(getWidth());

    // Held back until there are notes to sit on them, so an empty roll does not read as a grid the
    // transcription failed to fill.
    const float alpha = mProcessor->getInstrumentMixer()->getTotalNoteCount() > 0 ? 1.0f : EMPTY_LANE_ALPHA;

    for (int note = mKeyboard.getRangeStart(); note <= mKeyboard.getRangeEnd(); note++) {
        if (!mKeyboard.getRectangleForKey(note).intersects(local_bounds)) {
            continue;
        }

        const auto [lane_y, lane_height] = _getNoteHeightAndWidthPianoRoll(note);

        g.setColour((_isWhiteKey(note) ? nn::colours::laneWhite : nn::colours::laneBlack).withMultipliedAlpha(alpha));
        g.fillRect(0.0f, lane_y, width, lane_height);

        // An octave separator on each C, which is the only thing standing in for the vertical grid
        // the design deliberately does without.
        if (note % 12 == 0) {
            g.setColour(nn::colours::divOctave.withMultipliedAlpha(alpha));
            g.fillRect(0.0f, lane_y + lane_height - 1.0f, width, 1.0f);
        }
    }
}

void PianoRoll::_drawNotes(Graphics& g) const
{
    const InstrumentMixer* mixer = mProcessor->getInstrumentMixer();
    const auto height = static_cast<float>(getHeight());

    for (const NoteEvent& note_event: mProcessor->getTranscriptionManager()->getNoteEventVector()) {
        // The range always covers the whole transcription, so this only skips a note in the window
        // between it arriving and the keyboard being told about it -- and the keyboard asserts on
        // a key it has no position for.
        if (note_event.pitch < mKeyboard.getRangeStart() || note_event.pitch > mKeyboard.getRangeEnd()) {
            continue;
        }

        const auto [note_y, note_height] = _getNoteHeightAndWidthPianoRoll(note_event.pitch);

        if (note_y < 0 || note_height >= height) {
            continue;
        }

        const double end_time = note_event.isDrum()
                                    ? std::max(note_event.endTime, note_event.startTime + DRUM_MIN_DRAWN_SECONDS)
                                    : note_event.endTime;

        const float x = _timeToPixel(static_cast<float>(note_event.startTime));
        const float width = std::max(1.0f, _timeToPixel(static_cast<float>(end_time)) - x - 1.0f);

        const juce::Colour colour = mixer->colourForProgram(note_event.program);
        const float alpha = mixer->isAudible(note_event.program) ? 1.0f : MUTED_NOTE_ALPHA;

        g.setColour(colour.withAlpha(alpha));
        g.fillRoundedRectangle(x, note_y, width, note_height, NOTE_CORNER);

        // A note-on marker. Without it a run of repeated notes at one pitch reads as one long one.
        if (width > 2.0f * ONSET_EDGE_WIDTH) {
            g.setColour(nn::colours::noteOnsetEdge().withMultipliedAlpha(alpha));
            g.fillRect(x, note_y, ONSET_EDGE_WIDTH, note_height);
        }
    }
}

void PianoRoll::_drawTranscriptionFrontier(Graphics& g) const
{
    if (mProcessor->getState() != Processing) {
        return;
    }

    // Past this line nothing has been decoded yet, so the piano roll is empty and the synth is
    // silent. Shading it says which, rather than leaving the two indistinguishable from a
    // transcription that simply found nothing there.
    const auto finalized_through = mProcessor->getTranscriptionManager()->getFinalizedThrough();
    const float frontier_x = _timeToPixel(static_cast<float>(finalized_through));
    const auto width = static_cast<float>(getWidth());

    if (frontier_x >= width) {
        return;
    }

    g.setColour(nn::colours::bgRoot.withAlpha(0.75f));
    g.fillRect(frontier_x, 0.0f, width - frontier_x, static_cast<float>(getHeight()));

    g.setColour(nn::colours::divStrong);
    g.drawVerticalLine(static_cast<int>(std::round(frontier_x)), 0.0f, static_cast<float>(getHeight()));
}

void PianoRoll::changeListenerCallback(ChangeBroadcaster* source)
{
    // Both change what is drawn here: the keyboard scrolling moves every lane, and the mixer can
    // change a note's colour or dim a whole instrument.
    if (source == &mKeyboard || source == mProcessor->getInstrumentMixer()) {
        repaint();
    }
}

void PianoRoll::mouseDown(const MouseEvent& event)
{
    mPlayhead.setPlayheadTime(_pixelToTime(static_cast<float>(event.x)));
}

void PianoRoll::_onVBlankCallback()
{
    if (!mPlayhead.isVisible()) {
        return;
    }

    const auto playhead_x = static_cast<int>(std::round(mPlayhead.getPlayheadX()));

    if (playhead_x == mLastPlayheadX) {
        return;
    }

    const int from = std::min(playhead_x, mLastPlayheadX);
    const int to = std::max(playhead_x, mLastPlayheadX);
    mLastPlayheadX = playhead_x;

    repaint(from - 2, 0, to - from + 4, getHeight());
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
    const int range_start = mKeyboard.getRangeStart();
    const int range_end = mKeyboard.getRangeEnd();

    jassert(inNote >= range_start && inNote <= range_end);

    // The lanes at the two ends have no neighbour on one side, so they take the space between
    // themselves and the one neighbour they do have.
    if (inNote == range_start) {
        return {
            _noteBottomY(inNote + 1),
            _noteBottomY(inNote) - _noteBottomY(inNote + 1),
        };
    } else if (inNote == range_end) {
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
    // Measured off the keyboard rather than off this component: the key positions it returns are
    // relative to its own extent, and the two only line up because they are laid out to the same
    // height. Reading it from the source removes the "only because" from that sentence.
    return static_cast<float>(mKeyboard.getHeight()) - mKeyboard.getKeyStartPosition(inNote);
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
