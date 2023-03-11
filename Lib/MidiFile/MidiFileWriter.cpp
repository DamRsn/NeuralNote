//
// Created by Damien Ronssin on 11.03.23.
//

#include "MidiFileWriter.h"

bool MidiFileWriter::writeMidiFile(const std::vector<Notes::Event>& inNoteEvents,
                                   juce::File& fileToUse,
                                   int inTempoBPM)
{
    juce::MidiMessageSequence message_sequence;

    auto tempo_meta_event = juce::MidiMessage::tempoMetaEvent(
        static_cast<float>(_BPMToMicrosecondsPerQuarterNote(inTempoBPM)));
    tempo_meta_event.setTimeStamp(0.0);
    message_sequence.addEvent(tempo_meta_event);

    auto time_signature_meta_event = juce::MidiMessage::timeSignatureMetaEvent(4, 4);
    time_signature_meta_event.setTimeStamp(0.0);
    message_sequence.addEvent(time_signature_meta_event);

    for (auto& note: inNoteEvents)
    {
        auto note_on =
            juce::MidiMessage::noteOn(1, note.pitch, static_cast<float>(note.amplitude));
        note_on.setTimeStamp(note.start * inTempoBPM / 60.0 * 960);

        auto note_off = juce::MidiMessage::noteOff(1, note.pitch);
        note_off.setTimeStamp(note.end * inTempoBPM / 60.0 * 960);

        message_sequence.addEvent(note_on);

        message_sequence.addEvent(note_off);
        message_sequence.updateMatchedPairs();
    }

    message_sequence.sort();
    message_sequence.updateMatchedPairs();

    juce::MidiFile midi_file;

    midi_file.setTicksPerQuarterNote(960);

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
