//
// Created by Damien Ronssin on 11.03.23.
//

#include "MidiFileWriter.h"

#include <algorithm>
#include <array>

#include "InstrumentInfo.h"

namespace {
// 1-based, as JUCE counts MIDI channels.
constexpr int DRUM_CHANNEL = 10;

constexpr std::array<int, 15> MELODIC_CHANNELS = {1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 12, 13, 14, 15, 16};

// Where ReuseChannels starts handing channels out again: the block above the drums, so a reused
// channel is always one of the last few rather than colliding with the first instrument found.
constexpr size_t REUSE_FIRST_INDEX = 9;

// Channel 10 selects the kit from the note number, so the program there names the kit rather than
// the instrument. 0 is the standard kit.
constexpr int DRUM_KIT_PROGRAM = 0;

// Meta event type for a track name.
constexpr int TRACK_NAME_META_EVENT = 3;
} // namespace

MidiChannelMap buildChannelMap(const std::vector<NoteEvent>& inNoteEvents, MidiOverflowMode inMode)
{
    std::map<int, size_t> note_counts;
    for (const auto& note: inNoteEvents) {
        note_counts[note.program]++;
    }

    // Ascending, which matches InstrumentMixer's ordering in the sidebar.
    std::vector<int> melodic_programs;
    for (const auto& entry: note_counts) {
        if (entry.first != msl::DRUM_PROGRAM) {
            melodic_programs.push_back(entry.first);
        }
    }

    MidiChannelMap channel_map;

    if (inMode == MidiOverflowMode::DropExtraInstruments && melodic_programs.size() > MELODIC_CHANNELS.size()) {
        // Keep whatever carries the most notes: a lead is worth more than an incidental instrument
        // with three notes, whatever their program numbers say.
        const auto more_notes_first = [&note_counts](int inLhs, int inRhs) {
            const size_t lhs_count = note_counts.at(inLhs);
            const size_t rhs_count = note_counts.at(inRhs);
            return lhs_count != rhs_count ? lhs_count > rhs_count : inLhs < inRhs;
        };

        std::sort(melodic_programs.begin(), melodic_programs.end(), more_notes_first);

        channel_map.droppedPrograms.assign(melodic_programs.begin() + MELODIC_CHANNELS.size(), melodic_programs.end());
        std::sort(channel_map.droppedPrograms.begin(), channel_map.droppedPrograms.end());

        melodic_programs.resize(MELODIC_CHANNELS.size());
        std::sort(melodic_programs.begin(), melodic_programs.end());
    }

    for (size_t i = 0; i < melodic_programs.size(); i++) {
        if (i < MELODIC_CHANNELS.size()) {
            channel_map.channelForProgram[melodic_programs[i]] = MELODIC_CHANNELS[i];
        } else {
            const size_t reuse_span = MELODIC_CHANNELS.size() - REUSE_FIRST_INDEX;
            const size_t overflow_index = i - MELODIC_CHANNELS.size();
            channel_map.channelForProgram[melodic_programs[i]] =
                MELODIC_CHANNELS[REUSE_FIRST_INDEX + overflow_index % reuse_span];
        }
    }

    if (note_counts.count(msl::DRUM_PROGRAM) > 0) {
        channel_map.channelForProgram[msl::DRUM_PROGRAM] = DRUM_CHANNEL;
    }

    return channel_map;
}

bool MidiFileWriter::writeMidiFile(const std::vector<NoteEvent>& inNoteEvents,
                                   const File& fileToUse,
                                   double inExportBpm,
                                   double inStartOffsetSeconds,
                                   MidiOverflowMode inOverflowMode) const
{
    jassert(inStartOffsetSeconds >= 0.0);

    const auto channel_map = buildChannelMap(inNoteEvents, inOverflowMode);

    const auto to_ticks = [&](double inTimeSeconds) {
        return (inTimeSeconds + inStartOffsetSeconds) * inExportBpm / 60.0 * mTicksPerQuarterNote;
    };

    MidiFile midi_file;
    midi_file.setTicksPerQuarterNote(mTicksPerQuarterNote);

    // Track 0 is the conductor track: tempo and meter, no notes.
    MidiMessageSequence conductor_track;

    auto tempo_meta_event =
        MidiMessage::tempoMetaEvent(static_cast<int>(std::round(_BPMToMicrosecondsPerQuarterNote(inExportBpm))));
    tempo_meta_event.setTimeStamp(0.0);
    conductor_track.addEvent(tempo_meta_event);

    // A placeholder: the model gives no meter.
    auto time_signature_meta_event = MidiMessage::timeSignatureMetaEvent(4, 4);
    time_signature_meta_event.setTimeStamp(0.0);
    conductor_track.addEvent(time_signature_meta_event);

    midi_file.addTrack(conductor_track);

    // One track per instrument: many DAWs split an import by track and re-channel it, so the track
    // is what reliably carries the instrument identity across.
    std::map<int, MidiMessageSequence> instrument_tracks;

    for (const auto& [program, channel]: channel_map.channelForProgram) {
        auto& track = instrument_tracks[program];

        auto track_name = MidiMessage::textMetaEvent(TRACK_NAME_META_EVENT, instrumentDisplayFor(program).name);
        track_name.setTimeStamp(0.0);
        track.addEvent(track_name);

        auto program_change =
            MidiMessage::programChange(channel, program == msl::DRUM_PROGRAM ? DRUM_KIT_PROGRAM : program);
        program_change.setTimeStamp(0.0);
        track.addEvent(program_change);
    }

    for (const auto& note: inNoteEvents) {
        const auto channel_it = channel_map.channelForProgram.find(note.program);
        if (channel_it == channel_map.channelForProgram.end()) {
            continue;
        }

        const int channel = channel_it->second;
        auto& track = instrument_tracks[note.program];

        auto note_on = MidiMessage::noteOn(channel, note.pitch, static_cast<float>(note.amplitude));
        note_on.setTimeStamp(to_ticks(note.startTime));

        auto note_off = MidiMessage::noteOff(channel, note.pitch);
        note_off.setTimeStamp(to_ticks(note.endTime));

        track.addEvent(note_on);
        track.addEvent(note_off);
    }

    for (auto& [program, track]: instrument_tracks) {
        track.updateMatchedPairs();
        midi_file.addTrack(track);
    }

    FileOutputStream output_stream(fileToUse);

    if (!output_stream.openedOk())
        return false;

    output_stream.setPosition(0);
    // Without this, overwriting a longer file leaves its tail behind the new one.
    if (!output_stream.truncate().wasOk())
        return false;

    return midi_file.writeTo(output_stream);
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
