//
// Created by Damien Ronssin on 04.08.26.
//

#ifndef NoteScheduler_h
#define NoteScheduler_h

#include <array>
#include <atomic>
#include <cstddef>
#include <span>
#include <vector>

#include <JuceHeader.h>

#include "NoteEvent.h"
#include "SynthEvent.h"

/**
 * Turns a note list into note-ons and note-offs, block by block.
 *
 * It knows nothing about what plays the result. It produces two renderings of the same decisions --
 * a MidiBuffer for the plugin's MIDI output port, and a SynthEvent list for the internal synth --
 * so there is exactly one place that decides when a note stops, whatever ends up consuming it.
 *
 * The two exist separately because MIDI has 16 channels and a transcription can name 35
 * instruments. The MidiBuffer flattens; the SynthEvent list carries the instrument. Both are
 * written from the same table in the same pass, so they cannot disagree.
 *
 * Its invariant: **every note-on it emits is followed by a matching note-off**, on both outputs.
 * Note-offs are generated from a table of what this class started, not looked up in the note list,
 * so nothing that happens to that list can strand a sounding note. Since a NoteEvent always has a
 * finite end, a hanging note would need a table entry with no end time, which cannot be created.
 *
 * That matters because the note list is replaced repeatedly while a transcription streams in, and
 * again whenever a post-processing parameter changes.
 *
 * No synchronisation of its own: renderNextBlock is the audio thread's, and every other method must
 * be called either from that thread or with it held off. SynthController is what arranges that.
 */
class NoteScheduler
{
public:
    NoteScheduler();

    void setSampleRate(double inSampleRate);

    /**
     * Swaps in a new note list, which must be sorted by start time.
     *
     * Notes already sounding survive if the new list still has them; the rest stop in the next
     * block. A note is never left sounding because it went missing from the list.
     */
    void setNotes(std::vector<NoteEvent>& ioNotes);

    /**
     * Appends this block's MIDI to outMidiBuffer, and rebuilds the SynthEvent list that
     * getSynthEvents returns.
     *
     * @param inNumSamples Block length.
     * @param inIsPlaying Whether the transport is running. When it is not, time does not advance
     *        and nothing new starts, but anything still sounding is stopped.
     */
    void renderNextBlock(MidiBuffer& outMidiBuffer, int inNumSamples, bool inIsPlaying);

    /**
     * @return The last renderNextBlock call's events, in nondecreasing sample offset, with a
     *         note-off always ahead of a note-on it shares an offset with. Valid until the next
     *         call.
     */
    std::span<const SynthEvent> getSynthEvents() const { return {mSynthEvents.data(), mSynthEvents.size()}; }

    /** Stops everything currently sounding, in the next block. */
    void stopAllNotes();

    /**
     * Emits a note-off for everything currently sounding into a caller-supplied buffer, without
     * stopping it here. For a consumer being disconnected while the rest keep playing.
     */
    void emitActiveNotesOffTo(MidiBuffer& outMidiBuffer) const;

    /** Moves the playhead. Anything sounding is stopped: it does not belong where the playhead now is. */
    void setTimeSeconds(double inNewTime);

    double getTimeSeconds() const;

    /** Rewinds to the start and releases everything sounding, keeping the note list. */
    void reset();

    /** @return Whether the current note list has anything in it. */
    bool hasNotes() const { return !mNotes.empty(); }

    /** @return How many notes are sounding. For tests; the invariant is that this reaches 0. */
    std::size_t getNumActiveNotes() const { return mNumActive; }

    // Also the polyphony ceiling. Reaching it steals the oldest note rather than dropping a
    // note-off. The old bound argued that 128 pitches on one channel cannot exceed 256; that no
    // longer holds now that 35 instruments each have 128 pitches, so this is sized for headroom
    // rather than derived. Well past what a transcription sustains at once, and cheap either way.
    static constexpr std::size_t MAX_ACTIVE_NOTES = 512;

private:
    /** A note this class has started and not yet stopped. */
    struct ActiveNote {
        int program = 0;
        int pitch = 0;
        double endTime = 0.0;
    };

    void _updateCursor();

    /** Appends to both outputs at once, so neither can be given an event the other was not. */
    void _emitNoteOn(MidiBuffer& outMidiBuffer, const NoteEvent& inNote, int inSampleOffset);

    /** Drops the entry at inIndex, emitting its note-off first. Keeps the rest in start order. */
    void _stopActive(MidiBuffer& outMidiBuffer, std::size_t inIndex, int inSampleOffset);

    /** @return The index of the sounding note on inProgram/inPitch, or mNumActive if there is none. */
    std::size_t _findActive(int inProgram, int inPitch) const;

    /**
     * Puts the block's events back into sample order. They are appended in the order the block's
     * passes happen -- expiry walks the active table in start order, not end order -- so the
     * offsets come out shuffled. Insertion sort because it is stable, which is what keeps a
     * note-off ahead of the note-on that shares its offset, and because the list is short and
     * nearly sorted already.
     */
    void _sortSynthEvents();

    /** Stops everything that has ended before inLimit. */
    void _expireBefore(MidiBuffer& outMidiBuffer, double inLimit, int inNumSamples);

    /**
     * Starts the notes the playhead is currently inside, after a seek or a resume. Without it,
     * landing in the middle of a held chord would be silent until the next onset.
     */
    void _startNotesCovering(MidiBuffer& outMidiBuffer);

    /**
     * @return The end time of the note in the current list that covers inTime on inProgram/inPitch,
     *         or a negative value if there is none -- which stops the sounding note in the next
     *         block.
     */
    double _endTimeOfCoveringNote(int inProgram, int inPitch, double inTime) const;

    int _sampleOffsetFor(double inTime, int inNumSamples) const;

    // Sorted by startTime.
    std::vector<NoteEvent> mNotes;

    // Index of the first note starting at or after mCurrentTime.
    std::size_t mCursor = 0;

    // Kept in the order notes started, so index 0 is the oldest.
    std::array<ActiveNote, MAX_ACTIVE_NOTES> mActive {};
    std::size_t mNumActive = 0;

    // Rebuilt every block. Reserved generously up front rather than bounded: how many notes start
    // inside one block has no ceiling, and dropping an event to respect a fixed one could drop a
    // note-off. MidiBuffer is pre-sized the same way, for the same reason.
    std::vector<SynthEvent> mSynthEvents;

    // How many instruments are sounding each pitch. The MidiBuffer flattens them onto one channel,
    // where a pitch can only be on or off, so it gets a note-on when this rises from zero and a
    // note-off when it returns. Retires when instruments get their own MIDI channels.
    std::array<int, 128> mMidiPitchCount {};

    // Raised when the playhead moves or the transport stops. The next block releases everything
    // sounding; the next *playing* block then re-attacks whatever covers the new position, and
    // only then is it cleared -- so a pause holds the flag until playback actually resumes.
    std::atomic<bool> mShouldResync = false;

    // Atomic because the UI reads the playhead position from the message thread.
    std::atomic<double> mCurrentTime = 0.0;

    double mSampleRate = 44100;
};

#endif // NoteScheduler_h
