//
// Created by Damien Ronssin on 04.08.26.
//

#include "NoteScheduler.h"

#include <algorithm>
#include <cmath>

namespace
{
// The MidiBuffer still flattens every instrument onto one channel. Mapping 35 instruments onto 16
// MIDI channels is its own decision, with its own doc, and it does not belong in the class that
// decides *when* notes happen. The SynthEvent list carries the instrument in the meantime, so
// nothing is lost while that is settled.
constexpr int MIDI_CHANNEL = 1;

// How far back setNotes looks for the note a sounding pitch came from. A covering note starts
// before the playhead, and proving one absent would otherwise mean scanning the whole history with
// the audio callback held off. Not finding a match only stops a note early, never leaves one
// sounding, so the bound errs in the harmless direction.
constexpr double MAX_LOOKBACK_SECONDS = 30.0;

// Enough for every active note to be released and a fresh set started without reallocating. Same
// sizing argument as SynthController's MidiBuffer.
constexpr std::size_t SYNTH_EVENT_CAPACITY = 4 * NoteScheduler::MAX_ACTIVE_NOTES;
} // namespace

NoteScheduler::NoteScheduler()
{
    mSynthEvents.reserve(SYNTH_EVENT_CAPACITY);
}

void NoteScheduler::setSampleRate(double inSampleRate)
{
    mSampleRate = inSampleRate;
}

void NoteScheduler::setNotes(std::vector<NoteEvent>& ioNotes)
{
    std::swap(ioNotes, mNotes);
    _updateCursor();

    // Re-anchor what is sounding to the new list rather than stopping it: a transcription streams
    // in a chunk at a time, and cutting every held note ~48 times over a song would be audible. A
    // note the new list no longer has gets an end time in the past, so the next block stops it
    // through the same path as any note reaching its end -- it cannot simply be forgotten.
    for (std::size_t i = 0; i < mNumActive; ++i) {
        mActive[i].endTime = _endTimeOfCoveringNote(mActive[i].program, mActive[i].pitch, mCurrentTime);
    }
}

void NoteScheduler::renderNextBlock(MidiBuffer& outMidiBuffer, int inNumSamples, bool inIsPlaying)
{
    mSynthEvents.clear();

    if (mShouldResync) {
        while (mNumActive > 0) {
            _stopActive(outMidiBuffer, mNumActive - 1, 0);
        }

        if (inIsPlaying) {
            // Only once playing: a stopped transport keeps the flag so that resuming re-attacks
            // what the playhead is sitting inside, instead of staying silent until the next onset.
            _startNotesCovering(outMidiBuffer);
            mShouldResync = false;
        }
    }

    if (!inIsPlaying) {
        // Time does not advance and nothing new starts, but the release above still had to run: a
        // transport stopping mid-note owes the synth and the host a note-off.
        _sortSynthEvents();
        return;
    }

    const double end_time = mCurrentTime + inNumSamples / mSampleRate;

    _expireBefore(outMidiBuffer, end_time, inNumSamples);

    while (mCursor < mNotes.size() && mNotes[mCursor].startTime < end_time) {
        const NoteEvent note = mNotes[mCursor];
        mCursor += 1;

        // Already over by the time we reached it -- post-processing can shorten a note under a
        // running playhead. Starting it would only produce a note-on chasing its own note-off.
        if (note.endTime <= mCurrentTime) {
            continue;
        }

        const int offset = _sampleOffsetFor(note.startTime, inNumSamples);

        // Only the same instrument's note on this pitch: two instruments playing the same note is
        // ordinary, and releasing the other one would silence it for the rest of its duration.
        const std::size_t sounding = _findActive(note.program, note.pitch);

        if (sounding < mNumActive) {
            // A retrigger has to release the previous one first, or the note-off that eventually
            // arrives reads as ending this one instead.
            _stopActive(outMidiBuffer, sounding, offset);
        }

        if (mNumActive == MAX_ACTIVE_NOTES) {
            _stopActive(outMidiBuffer, 0, offset);
        }

        mActive[mNumActive] = {note.program, note.pitch, note.endTime};
        mNumActive += 1;

        _emitNoteOn(outMidiBuffer, note, offset);
    }

    // Again, for notes that both start and end inside this block. A drum hit lasts 10 ms, which is
    // shorter than a block at most sizes, so without this pass they would all be stretched to one.
    _expireBefore(outMidiBuffer, end_time, inNumSamples);

    mCurrentTime = end_time;

    _sortSynthEvents();
}

void NoteScheduler::stopAllNotes()
{
    mShouldResync = true;
}

void NoteScheduler::_startNotesCovering(MidiBuffer& outMidiBuffer)
{
    const double time = mCurrentTime;
    const double earliest = time - MAX_LOOKBACK_SECONDS;

    // Backwards from the cursor, so the first match on an instrument and pitch is the latest note to
    // have started there -- the one actually sounding at this point.
    for (std::size_t i = mCursor; i-- > 0;) {
        const NoteEvent& note = mNotes[i];

        if (note.startTime < earliest) {
            break;
        }

        if (note.startTime > time || note.endTime <= time) {
            continue;
        }

        const bool already_sounding = _findActive(note.program, note.pitch) < mNumActive;

        if (already_sounding || mNumActive == MAX_ACTIVE_NOTES) {
            continue;
        }

        mActive[mNumActive] = {note.program, note.pitch, note.endTime};
        mNumActive += 1;

        _emitNoteOn(outMidiBuffer, note, 0);
    }
}

void NoteScheduler::emitActiveNotesOffTo(MidiBuffer& outMidiBuffer) const
{
    // Per open pitch, not per active note: what the host is holding is what went out on the wire,
    // and two instruments sharing a pitch only ever opened it once.
    for (std::size_t pitch = 0; pitch < mMidiPitchCount.size(); ++pitch) {
        if (mMidiPitchCount[pitch] > 0) {
            outMidiBuffer.addEvent(MidiMessage::noteOff(MIDI_CHANNEL, static_cast<int>(pitch)), 0);
        }
    }
}

void NoteScheduler::setTimeSeconds(double inNewTime)
{
    jassert(inNewTime >= 0);

    mCurrentTime = inNewTime;
    _updateCursor();

    mShouldResync = true;
}

double NoteScheduler::getTimeSeconds() const
{
    return mCurrentTime;
}

void NoteScheduler::reset()
{
    // Not dropped outright: whatever is sounding still needs its note-off, which the next block
    // delivers.
    setTimeSeconds(0.0);
}

void NoteScheduler::_updateCursor()
{
    mCursor = static_cast<std::size_t>(std::lower_bound(mNotes.begin(),
                                                        mNotes.end(),
                                                        mCurrentTime.load(),
                                                        [](const NoteEvent& a, double b) { return a.startTime < b; })
                                       - mNotes.begin());
}

void NoteScheduler::_emitNoteOn(MidiBuffer& outMidiBuffer, const NoteEvent& inNote, int inSampleOffset)
{
    const auto velocity = static_cast<float>(inNote.amplitude);

    mSynthEvents.push_back({inSampleOffset, inNote.program, inNote.pitch, velocity, true});

    // Only the first instrument to reach this pitch opens it on the wire. Two instruments playing
    // the same note is ordinary, but flattened onto one channel it would read as one note started
    // twice -- and the first note-off would then end both. Counting keeps the pitch on for as long
    // as anything is playing it, which is the best a single channel can represent. It goes away
    // once instruments get channels of their own.
    if (mMidiPitchCount[static_cast<std::size_t>(inNote.pitch)]++ == 0) {
        outMidiBuffer.addEvent(MidiMessage::noteOn(MIDI_CHANNEL, inNote.pitch, velocity), inSampleOffset);
    }
}

void NoteScheduler::_stopActive(MidiBuffer& outMidiBuffer, std::size_t inIndex, int inSampleOffset)
{
    jassert(inIndex < mNumActive);

    const int pitch = mActive[inIndex].pitch;

    mSynthEvents.push_back({inSampleOffset, mActive[inIndex].program, pitch, 0.0f, false});

    if (--mMidiPitchCount[static_cast<std::size_t>(pitch)] == 0) {
        outMidiBuffer.addEvent(MidiMessage::noteOff(MIDI_CHANNEL, pitch), inSampleOffset);
    }

    jassert(mMidiPitchCount[static_cast<std::size_t>(pitch)] >= 0);

    for (std::size_t i = inIndex + 1; i < mNumActive; ++i) {
        mActive[i - 1] = mActive[i];
    }

    mNumActive -= 1;
}

void NoteScheduler::_expireBefore(MidiBuffer& outMidiBuffer, double inLimit, int inNumSamples)
{
    std::size_t i = 0;

    while (i < mNumActive) {
        if (mActive[i].endTime < inLimit) {
            _stopActive(outMidiBuffer, i, _sampleOffsetFor(mActive[i].endTime, inNumSamples));
        } else {
            i += 1;
        }
    }
}

std::size_t NoteScheduler::_findActive(int inProgram, int inPitch) const
{
    for (std::size_t i = 0; i < mNumActive; ++i) {
        if (mActive[i].program == inProgram && mActive[i].pitch == inPitch) {
            return i;
        }
    }

    return mNumActive;
}

void NoteScheduler::_sortSynthEvents()
{
    for (std::size_t i = 1; i < mSynthEvents.size(); ++i) {
        const SynthEvent event = mSynthEvents[i];
        std::size_t j = i;

        // Strictly greater, so equal offsets keep the order they were emitted in.
        while (j > 0 && mSynthEvents[j - 1].sampleOffset > event.sampleOffset) {
            mSynthEvents[j] = mSynthEvents[j - 1];
            j -= 1;
        }

        mSynthEvents[j] = event;
    }
}

double NoteScheduler::_endTimeOfCoveringNote(int inProgram, int inPitch, double inTime) const
{
    const double earliest = inTime - MAX_LOOKBACK_SECONDS;

    // mCursor is the first note starting at or after inTime, so anything that could cover it is
    // behind that.
    for (std::size_t i = mCursor; i-- > 0;) {
        const NoteEvent& note = mNotes[i];

        if (note.startTime < earliest) {
            break;
        }

        if (note.program == inProgram && note.pitch == inPitch && note.startTime <= inTime
            && note.endTime > inTime) {
            return note.endTime;
        }
    }

    return -1.0;
}

int NoteScheduler::_sampleOffsetFor(double inTime, int inNumSamples) const
{
    // A host can hand us an empty block; std::clamp with lo > hi is undefined.
    if (inNumSamples <= 0) {
        return 0;
    }

    const double offset = (inTime - mCurrentTime) * mSampleRate;

    return std::clamp(static_cast<int>(std::round(offset)), 0, inNumSamples - 1);
}
