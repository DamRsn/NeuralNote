//
// Created by Damien Ronssin on 10.03.23.
//

#include "Keyboard.h"

#include "NnFonts.h"
#include "NnLook.h"

namespace
{
/** How far the key column falls back while the roll beside it is empty. */
constexpr float DIMMED_ALPHA = 0.4f;
} // namespace

Keyboard::Keyboard()
    : KeyboardComponentBase(juce::KeyboardComponentBase::Orientation::verticalKeyboardFacingRight)
{
    setAvailableRange(MIN_MIDI_NOTE, MAX_MIDI_NOTE);

    setBlackNoteWidthProportion(0.58f);
    setBlackNoteLengthProportion(0.65f);
    setKeyWidth(nn::metrics::pianoKeyHeight);

    // Zoomed in, the keyboard is taller than its column, and JUCE puts its own scroll arrows over
    // the keys. They are drawn as nothing (drawUpDownButton) and narrowed to a sliver rather than
    // turned off: setScrollButtonsVisible(false) also pins the view to the bottom of the range,
    // which would take the scrolling with it.
    setScrollButtonWidth(1);
}

void Keyboard::setDisplayedRange(const PianoRollRange::DisplayRange& inRange)
{
    // setAvailableRange is a no-op when nothing changes, but it is also what moves the visible
    // window, so it is worth not calling it on every mixer message.
    if (inRange != getDisplayedRange()) {
        setAvailableRange(inRange.lowNote, inRange.highNote);
    }
}

PianoRollRange::DisplayRange Keyboard::getDisplayedRange() const
{
    return {getRangeStart(), getRangeEnd()};
}

void Keyboard::setWhiteKeyHeight(float inHeightPx)
{
    if (juce::approximatelyEqual(inHeightPx, getKeyWidth())) {
        return;
    }

    // Zooming holds the middle of the view still. Without this, zooming in walks the view down to
    // the bottom of the range, because that is where JUCE keeps the scroll position anchored.
    const int centre_note = getLowestVisibleKey() + _visibleSemitones() / 2;

    setKeyWidth(inHeightPx);
    setLowestVisibleKey(centre_note - _visibleSemitones() / 2);

    // setKeyWidth only lays this component out again. The piano roll reads its geometry from here
    // and listens for change messages, so without this the two would disagree until something else
    // happened to send one.
    sendChangeMessage();
}

int Keyboard::_visibleSemitones() const
{
    if (getKeyWidth() <= 0.0f) {
        return nn::zoom::minVisibleSemitones;
    }

    // Height over one white key's height is white keys, and there are twelve semitones to seven of
    // those.
    return juce::roundToInt(static_cast<float>(getHeight()) / getKeyWidth()
                            * static_cast<float>(nn::zoom::semitonesPerOctave)
                            / static_cast<float>(nn::zoom::whiteKeysPerOctave));
}

void Keyboard::scrollByWheel(const MouseEvent& inEvent, const MouseWheelDetails& inWheel)
{
    mouseWheelMove(inEvent, inWheel);
}

void Keyboard::drawKeyboardBackground(Graphics& g, Rectangle<float> area)
{
    g.setColour(nn::colours::bgGutter);
    g.fillRect(area);

    g.setColour(nn::colours::divStrong);
    g.fillRect(area.removeFromRight(1.0f));
}

void Keyboard::setDimmed(bool inIsDimmed)
{
    if (inIsDimmed != mIsDimmed) {
        mIsDimmed = inIsDimmed;
        repaint();
    }
}

Colour Keyboard::_keyColour(Colour inColour) const
{
    // Blended into an opaque colour rather than drawn translucent: black keys overlap white keys,
    // and translucent ones would let the gaps between white keys show through.
    return mIsDimmed ? nn::colours::bgGutter.interpolatedWith(inColour, DIMMED_ALPHA) : inColour;
}

void Keyboard::drawWhiteKey(int midiNoteNumber, Graphics& g, Rectangle<float> area)
{
    // A 1 px gap along the bottom edge, which is what separates one key from the next: the design
    // has no key outlines, so the background showing through is the only divider.
    g.setColour(_keyColour(nn::colours::keyWhite));
    g.fillRect(area.withTrimmedBottom(1.0f).withTrimmedRight(1.0f));

    if (midiNoteNumber % 12 == 0) {
        const int octave_number = midiNoteNumber / 12 - 1;

        g.setColour(_keyColour(nn::colours::keyLabel));
        g.setFont(nn::fonts::scaleLabel());
        g.drawText("C" + String(octave_number),
                   area.withTrimmedLeft(getBlackNoteLength()).withTrimmedRight(5.0f),
                   juce::Justification::centredRight);
    }
}

void Keyboard::drawBlackKey(int midiNoteNumber, Graphics& g, Rectangle<float> area)
{
    ignoreUnused(midiNoteNumber);

    g.setColour(_keyColour(nn::colours::keyBlack));
    g.fillRect(area.withTrimmedBottom(1.0f));
}

void Keyboard::drawUpDownButton(Graphics& g, int w, int h, bool isMouseOver, bool isButtonPressed, bool movesOctavesUp)
{
    ignoreUnused(g, w, h, isMouseOver, isButtonPressed, movesOctavesUp);
}
