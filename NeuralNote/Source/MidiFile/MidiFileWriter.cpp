//
// Created by Damien Ronssin on 11.03.23.
//

#include "MidiFileWriter.h"

bool MidiFileWriter::writeMidiFile(const std::vector<Notes::Event>& inNoteEvents,
                                   const File& fileToUse,
                                   const TimeQuantizeOptions::TimeQuantizeInfo& inInfo,
                                   double inExportBpm,
                                   PitchBendModes inPitchBendMode) const
{
    // Compute offset to start at beginning of the previous bar
    const double start_offset = - inInfo.getStartLastBarSec();
    jassert(start_offset >= 0.0);

    MidiMessageSequence message_sequence;

    // Set tempo
    auto tempo_meta_event =
        MidiMessage::tempoMetaEvent(static_cast<int>(std::round(_BPMToMicrosecondsPerQuarterNote(inExportBpm))));
    tempo_meta_event.setTimeStamp(0.0);
    message_sequence.addEvent(tempo_meta_event);

    // Set time signature
    auto time_signature_meta_event =
        MidiMessage::timeSignatureMetaEvent(inInfo.timeSignatureNum, inInfo.timeSignatureDenom);
    time_signature_meta_event.setTimeStamp(0.0);
    message_sequence.addEvent(time_signature_meta_event);

    float prev_pitch_bend_semitone = 0.0f;

    // Add note events
    for (auto& note: inNoteEvents) {
        auto note_on = MidiMessage::noteOn(1, note.pitch, static_cast<float>(note.amplitude));
        note_on.setTimeStamp((note.startTime + start_offset) * inExportBpm / 60.0 * mTicksPerQuarterNote);

        auto note_off = MidiMessage::noteOff(1, note.pitch);
        note_off.setTimeStamp((note.endTime + start_offset) * inExportBpm / 60.0 * mTicksPerQuarterNote);

        message_sequence.addEvent(note_on);

        // Add pitch bend event
        if (inPitchBendMode == SinglePitchBend) {
            for (size_t i = 0; i < note.bends.size(); i++) {
                prev_pitch_bend_semitone = static_cast<float>(note.bends[i]) / 3.0f;
                auto pitch_wheel_pos = MidiMessage::pitchbendToPitchwheelPos(prev_pitch_bend_semitone, 4.0f);
                auto pitch_wheel_event = MidiMessage::pitchWheel(1, pitch_wheel_pos);
                pitch_wheel_event.setTimeStamp((note.startTime + start_offset + i * FFT_HOP / BASIC_PITCH_SAMPLE_RATE)
                                               * inExportBpm / 60.0 * mTicksPerQuarterNote);
                message_sequence.addEvent(pitch_wheel_event);
            }

            if (note.bends.empty() && prev_pitch_bend_semitone != 0) {
                prev_pitch_bend_semitone = 0.0f;
                auto pitch_wheel_pos = MidiMessage::pitchbendToPitchwheelPos(0.0f, 4.0f);
                auto pitch_wheel_event = MidiMessage::pitchWheel(1, pitch_wheel_pos);
                pitch_wheel_event.setTimeStamp((note.startTime + start_offset) * inExportBpm / 60.0
                                               * mTicksPerQuarterNote);
                message_sequence.addEvent(pitch_wheel_event);
            }
        }

        message_sequence.addEvent(note_off);
        message_sequence.updateMatchedPairs();
    }

    message_sequence.sort();
    message_sequence.updateMatchedPairs();

    DBG("Length of note vector: " << inNoteEvents.size());
    DBG("NumEvents in message sequence:" << message_sequence.getNumEvents());

    // Write midi file
    MidiFile midi_file;

    midi_file.setTicksPerQuarterNote(mTicksPerQuarterNote);

    midi_file.addTrack(message_sequence);

    FileOutputStream output_stream(fileToUse);

    if (!output_stream.openedOk())
        return false;

    output_stream.setPosition(0);

    bool success = midi_file.writeTo(output_stream);

    return success;
}

double MidiFileWriter::_BPMToMicrosecondsPerQuarterNote(double inTempoBPM)
{
    // Beats per second
    double bps = inTempoBPM / 60.0;
    // Seconds per beat
    double spb = 1 / bps;
    // Microseconds per beat
    return 1.0e6 * spb;
}
