//
// Created by Damien Ronssin on 11.03.23.
//

#include "MidiFileWriter.h"

bool MidiFileWriter::writeMidiFile(
    const std::vector<Notes::Event>& inNoteEvents,
    juce::File& fileToUse,
    const juce::AudioPlayHead::CurrentPositionInfo& inInfoStart,
    AudioSampleAcquisitionMode inSampleAcquisitionMode)
{
    double tempo = inInfoStart.bpm;

    juce::MidiMessageSequence message_sequence;

    // Set tempo
    auto tempo_meta_event = juce::MidiMessage::tempoMetaEvent(
        static_cast<int>(std::round(_BPMToMicrosecondsPerQuarterNote(tempo))));
    tempo_meta_event.setTimeStamp(0.0);
    message_sequence.addEvent(tempo_meta_event);

    // Set time signature
    auto time_signature_meta_event = juce::MidiMessage::timeSignatureMetaEvent(
        inInfoStart.timeSigNumerator, inInfoStart.timeSigDenominator);
    time_signature_meta_event.setTimeStamp(0.0);
    message_sequence.addEvent(time_signature_meta_event);

    // Add note events
    for (auto& note: inNoteEvents)
    {
        auto note_on =
            juce::MidiMessage::noteOn(1, note.pitch, static_cast<float>(note.amplitude));
        note_on.setTimeStamp(note.start * tempo / 60.0 * mTicksPerQuarterNote);

        auto note_off = juce::MidiMessage::noteOff(1, note.pitch);
        note_off.setTimeStamp(note.end * tempo / 60.0 * mTicksPerQuarterNote);

        message_sequence.addEvent(note_on);

        message_sequence.addEvent(note_off);
        message_sequence.updateMatchedPairs();
    }

    message_sequence.sort();
    message_sequence.updateMatchedPairs();

    DBG("Length of note vector: " << inNoteEvents.size());
    DBG("NumEvents in message sequence:" << message_sequence.getNumEvents());

    // Write midi file
    juce::MidiFile midi_file;

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
