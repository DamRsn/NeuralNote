//
// Created by Damien Ronssin on 06.08.26.
//

#ifndef SynthEvent_h
#define SynthEvent_h

/**
 * One note-on or note-off for the internal synth, at a sample offset in the current block.
 *
 * It exists because a MidiBuffer cannot carry what the synth needs. MIDI has 16 channels; a
 * transcription can name 35 instruments, and the mapping from one to the other is a product
 * decision with its own tradeoffs (see the MIDI section of the synth notes). The synth should not
 * inherit those tradeoffs -- it can play every instrument the model found, so it is given the
 * instrument directly.
 *
 * NoteScheduler emits these and the MidiBuffer from the same table, in the same pass, so the two
 * can never disagree about what is sounding.
 */
struct SynthEvent {
    int sampleOffset = 0;
    int program = 0; // 0-127, or msl::DRUM_PROGRAM
    int pitch = 0;
    float velocity = 0.0f; // 0-1; meaningless for a note-off
    bool isNoteOn = false;
};

#endif // SynthEvent_h
