//
// Created by Damien Ronssin on 10.06.23.
//

#ifndef SynthController_h
#define SynthController_h

#include <JuceHeader.h>

#include "NoteEvent.h"
#include "NoteScheduler.h"

class NeuralNoteAudioProcessor;

/**
 * Wires NoteScheduler into the plugin: owns the block's MidiBuffer, keeps the audio thread out of
 * the way while the note list is swapped, and stops the transport at the end of the audio.
 *
 * The scheduling itself, including the guarantee that no note is ever left hanging, is
 * NoteScheduler's -- and deliberately knows nothing about the synth, so the buffer it produces is
 * equally good for the MIDI output port or a sampler replacing the current one.
 */
class SynthController
{
public:
    explicit SynthController(NeuralNoteAudioProcessor* inProcessor);

    void setSampleRate(double inSampleRate);

    /**
     * Swaps in a new note list, sorted by start time. Takes the audio callback lock, so it may be
     * called from the message thread or a worker, but not from the audio thread.
     */
    void setNotes(std::vector<NoteEvent>& ioNotes);

    /**
     * Builds this block's MIDI. Audio thread only, and called every block whether or not the
     * transport is running -- a stopped transport still has note-offs to deliver.
     */
    const MidiBuffer& generateNextMidiBuffer(int inNumSamples, bool inIsPlaying);

    /**
     * @return The same events as the buffer generateNextMidiBuffer just returned, but carrying the
     *         instrument, for the internal synth. Valid until the next call.
     */
    std::span<const SynthEvent> getSynthEvents() const { return mScheduler.getSynthEvents(); }

    /** Stops everything currently sounding, in the next block. Thread-safe. */
    void stopAllNotes();

    /**
     * Emits a note-off for everything currently sounding into a caller-supplied buffer, without
     * stopping it here. For the one consumer the shared buffer cannot serve: MIDI output switched
     * off mid-note, after which the host stops receiving what this produces and would otherwise
     * hold those notes forever. The internal synth carries on.
     */
    void emitActiveNotesOffTo(MidiBuffer& outMidiBuffer) const;

    void reset();

    /** @return Whether there is anything to play. Audio thread safe: the list only changes under the callback lock. */
    bool hasNotes() const { return mScheduler.hasNotes(); }

    void setNewTimeSeconds(double inNewTime);

    double getCurrentTimeSeconds() const;

private:
    NeuralNoteAudioProcessor* mProcessor;

    NoteScheduler mScheduler;

    MidiBuffer mMidiBuffer;
};

#endif // SynthController_h
