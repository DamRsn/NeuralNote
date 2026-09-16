#include <JuceHeader.h>

#include <cmath>
#include <cstdio>
#include <map>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "NoteEvent.h"
#include "NoteScheduler.h"
#include "PianoRollRange.h"
#include "SynthEvent.h"
#include "WaveformBars.h"
#include "WaveformPeaks.h"

// NoteScheduler is the one place that decides when a note stops, for the synth and for the plugin's
// MIDI output alike, and it does so while the note list is replaced under it every few seconds by a
// streaming transcription. Its invariant -- every note-on is followed by a matching note-off -- is
// what these check, by driving it block by block and tracking what a consumer would be holding.
//
// The rest of the plugin still has no coverage; see the workspace's todo.

namespace
{

constexpr double SAMPLE_RATE = 48000.0;
constexpr int BLOCK_SIZE = 512;
constexpr double BLOCK_SECONDS = BLOCK_SIZE / SAMPLE_RATE;

int gFailures = 0;

void check(bool inCondition, const std::string& inWhat)
{
    if (!inCondition) {
        gFailures += 1;
        std::printf("FAIL: %s\n", inWhat.c_str());
    }
}

NoteEvent makeNote(double inStart, double inEnd, int inPitch, int inProgram = 0)
{
    NoteEvent note;
    note.startTime = inStart;
    note.endTime = inEnd;
    note.pitch = inPitch;
    note.amplitude = 100.0 / 127.0;
    note.program = inProgram;
    return note;
}

// Two arbitrary distinct melodic instruments, and drums, for the multi-instrument tests.
constexpr int PIANO = 0;
constexpr int BASS = 33;
constexpr int DRUMS = msl::DRUM_PROGRAM;

/** What a synth or a host would be holding, rebuilt from the buffers the scheduler produced. */
class SoundingNotes
{
public:
    void consume(const MidiBuffer& inBuffer)
    {
        for (const auto& metadata: inBuffer) {
            const MidiMessage message = metadata.getMessage();

            if (message.isNoteOn()) {
                mCounts[message.getNoteNumber()] += 1;
                check(mCounts[message.getNoteNumber()] <= 1,
                      "note " + std::to_string(message.getNoteNumber()) + " started twice without stopping");
            } else if (message.isNoteOff()) {
                mCounts[message.getNoteNumber()] -= 1;
                check(mCounts[message.getNoteNumber()] >= 0,
                      "note " + std::to_string(message.getNoteNumber()) + " stopped without starting");
            }
        }
    }

    int total() const
    {
        int sum = 0;

        for (const auto& [pitch, count]: mCounts) {
            sum += count;
        }

        return sum;
    }

private:
    std::map<int, int> mCounts;
};

/**
 * The same, for the synth's side of the output. Keyed on the instrument as well as the pitch,
 * because that is what the synth can tell apart and the MidiBuffer cannot.
 */
class SoundingVoices
{
public:
    void consume(std::span<const SynthEvent> inEvents)
    {
        int previous_offset = 0;

        for (const SynthEvent& event: inEvents) {
            check(event.sampleOffset >= previous_offset, "synth events are in sample order");
            previous_offset = event.sampleOffset;

            const auto key = std::make_pair(event.program, event.pitch);

            if (event.isNoteOn) {
                mCounts[key] += 1;
                check(mCounts[key] <= 1, "voice started twice without stopping");
            } else {
                mCounts[key] -= 1;
                check(mCounts[key] >= 0, "voice stopped without starting");
            }
        }
    }

    int total() const
    {
        int sum = 0;

        for (const auto& [key, count]: mCounts) {
            sum += count;
        }

        return sum;
    }

    int countFor(int inProgram) const
    {
        int sum = 0;

        for (const auto& [key, count]: mCounts) {
            if (key.first == inProgram) {
                sum += count;
            }
        }

        return sum;
    }

private:
    std::map<std::pair<int, int>, int> mCounts;
};

/** Runs blocks until inSeconds of transport time has passed, feeding everything to ioSounding. */
void runFor(NoteScheduler& ioScheduler,
            SoundingNotes& ioSounding,
            double inSeconds,
            bool inIsPlaying = true,
            SoundingVoices* ioVoices = nullptr)
{
    MidiBuffer buffer;

    for (int i = 0; i < static_cast<int>(std::ceil(inSeconds / BLOCK_SECONDS)); ++i) {
        buffer.clear();
        ioScheduler.renderNextBlock(buffer, BLOCK_SIZE, inIsPlaying);
        ioSounding.consume(buffer);

        if (ioVoices != nullptr) {
            ioVoices->consume(ioScheduler.getSynthEvents());
        }
    }
}

void setUp(NoteScheduler& ioScheduler, std::vector<NoteEvent> inNotes)
{
    ioScheduler.setSampleRate(SAMPLE_RATE);
    ioScheduler.setNotes(inNotes);
}

void testNotesStartAndStop()
{
    NoteScheduler scheduler;
    setUp(scheduler, {makeNote(0.1, 0.5, 60), makeNote(0.2, 0.3, 64)});
    SoundingNotes sounding;

    runFor(scheduler, sounding, 0.25);
    check(sounding.total() == 2, "both notes sounding mid-flight");

    runFor(scheduler, sounding, 0.5);
    check(sounding.total() == 0, "everything released once the notes are over");
    check(scheduler.getNumActiveNotes() == 0, "scheduler holds nothing after the notes are over");
}

void testShortNoteDoesNotSpanTwoBlocks()
{
    // A drum hit is 10 ms, shorter than the 10.67 ms block used here. One that fits entirely
    // inside a block has to start and stop within it, or every hit would be stretched to a block.
    NoteScheduler scheduler;
    setUp(scheduler, {makeNote(0.001, 0.009, 38)});

    MidiBuffer buffer;
    scheduler.renderNextBlock(buffer, BLOCK_SIZE, true);

    SoundingNotes sounding;
    sounding.consume(buffer);

    check(sounding.total() == 0, "a 10 ms note is over within one block");
}

void testSwapKeepsHeldNoteAndStopsRemovedOne()
{
    // What a streaming transcription does ~48 times a song: replace the list under a running
    // playhead. A note the new list still has must not be re-articulated; one it dropped must stop.
    NoteScheduler scheduler;
    setUp(scheduler, {makeNote(0.1, 10.0, 60), makeNote(0.1, 10.0, 67)});
    SoundingNotes sounding;

    runFor(scheduler, sounding, 0.3);
    check(sounding.total() == 2, "both long notes sounding before the swap");

    std::vector<NoteEvent> replacement = {makeNote(0.1, 10.0, 60)};
    scheduler.setNotes(replacement);

    runFor(scheduler, sounding, 0.05);
    check(sounding.total() == 1, "the dropped note stopped, the surviving one did not restart");

    runFor(scheduler, sounding, 10.0);
    check(sounding.total() == 0, "the survivor still stops at its own end time");
}

void testSwapToEmptyListStopsEverything()
{
    // The cancel and clear path: the transcription is thrown away while notes are sounding.
    NoteScheduler scheduler;
    setUp(scheduler, {makeNote(0.1, 30.0, 60), makeNote(0.1, 30.0, 64)});
    SoundingNotes sounding;

    runFor(scheduler, sounding, 0.3);
    check(sounding.total() == 2, "notes sounding before the list is dropped");

    std::vector<NoteEvent> nothing;
    scheduler.setNotes(nothing);

    runFor(scheduler, sounding, 0.05);
    check(sounding.total() == 0, "dropping the note list releases everything");
}

void testStopAndSeekRelease()
{
    NoteScheduler scheduler;
    setUp(scheduler, {makeNote(0.1, 30.0, 60)});
    SoundingNotes sounding;

    runFor(scheduler, sounding, 0.3);
    check(sounding.total() == 1, "note sounding before the transport stops");

    scheduler.stopAllNotes();
    runFor(scheduler, sounding, BLOCK_SECONDS, false);
    check(sounding.total() == 0, "stopping the transport releases what was sounding");

    runFor(scheduler, sounding, 0.3);
    check(sounding.total() == 1, "and it sounds again once playing resumes");

    scheduler.setTimeSeconds(20.0);
    runFor(scheduler, sounding, 0.05);
    check(sounding.total() == 1, "a seek inside the note re-articulates rather than doubling it");

    scheduler.setTimeSeconds(29.99);
    runFor(scheduler, sounding, 0.5);
    check(sounding.total() == 0, "and the note still ends where it ends");
}

void testResetReleases()
{
    NoteScheduler scheduler;
    setUp(scheduler, {makeNote(0.1, 30.0, 60)});
    SoundingNotes sounding;

    runFor(scheduler, sounding, 0.3);
    check(sounding.total() == 1, "note sounding before reset");

    scheduler.reset();
    runFor(scheduler, sounding, BLOCK_SECONDS, false);
    check(sounding.total() == 0, "reset releases what was sounding");
    check(scheduler.getTimeSeconds() == 0.0, "reset rewinds");
}

void testRetriggerReleasesFirst()
{
    // Back-to-back notes on one pitch. Without an explicit release the second note-on and the first
    // note-off would cancel out, and the pitch would be left sounding.
    NoteScheduler scheduler;
    setUp(scheduler, {makeNote(0.1, 0.2, 60), makeNote(0.2, 0.3, 60)});
    SoundingNotes sounding;

    runFor(scheduler, sounding, 0.25);
    check(sounding.total() == 1, "exactly one instance of the pitch sounding across the retrigger");

    runFor(scheduler, sounding, 0.2);
    check(sounding.total() == 0, "and it is released at the end");
}

void testPolyphonyCeilingStealsRatherThanLeaks()
{
    // More simultaneous notes than the table holds. Stealing has to emit the note-off it is
    // replacing, or the ceiling itself becomes a source of hanging notes.
    std::vector<NoteEvent> notes;

    for (std::size_t i = 0; i < NoteScheduler::MAX_ACTIVE_NOTES + 32; ++i) {
        notes.push_back(makeNote(0.1, 30.0, static_cast<int>(i % 128)));
    }

    NoteScheduler scheduler;
    setUp(scheduler, notes);
    SoundingNotes sounding;

    runFor(scheduler, sounding, 0.3);
    check(sounding.total() <= static_cast<int>(NoteScheduler::MAX_ACTIVE_NOTES),
          "never more sounding than the table can track");

    scheduler.stopAllNotes();
    runFor(scheduler, sounding, BLOCK_SECONDS, false);
    check(sounding.total() == 0, "and all of them can still be released");
}

void testStreamingSwapsNeverLeaveANoteSounding()
{
    // The whole feature end to end: a note list that grows a chunk at a time under a running
    // playhead, then stops. Nothing may be left sounding.
    std::vector<NoteEvent> notes;
    NoteScheduler scheduler;
    setUp(scheduler, {});
    SoundingNotes sounding;

    for (int chunk = 0; chunk < 12; ++chunk) {
        for (int i = 0; i < 20; ++i) {
            const double start = chunk * 5.0 + i * 0.25;
            // A mix of short hits and notes running across the next chunk boundary.
            notes.push_back(makeNote(start, start + (i % 3 == 0 ? 0.01 : 6.0), 40 + (i * 7) % 48));
        }

        std::vector<NoteEvent> copy = notes;
        scheduler.setNotes(copy);
        runFor(scheduler, sounding, 5.0);
    }

    scheduler.stopAllNotes();
    runFor(scheduler, sounding, BLOCK_SECONDS, false);

    check(sounding.total() == 0, "nothing left sounding after a full streaming run");
    check(scheduler.getNumActiveNotes() == 0, "and the scheduler agrees");
}

void testTwoInstrumentsShareAPitch()
{
    // Two instruments on the same note is ordinary music. Keyed on pitch alone -- as this was
    // before instruments existed -- the second note-on would release the first.
    NoteScheduler scheduler;
    setUp(scheduler, {makeNote(0.1, 1.0, 60, PIANO), makeNote(0.2, 1.0, 60, BASS)});
    SoundingNotes sounding;
    SoundingVoices voices;

    runFor(scheduler, sounding, 0.5, true, &voices);
    check(voices.total() == 2, "both instruments sounding the shared pitch");
    check(voices.countFor(PIANO) == 1, "the piano note survived the bass starting on its pitch");

    // The MidiBuffer has one channel, where a pitch is simply on or off, so it can only report the
    // pitch as open once. What it must not do is report it twice and then close it early --
    // SoundingNotes checks exactly that as it consumes.
    check(sounding.total() == 1, "flattened MIDI holds the shared pitch open once");

    runFor(scheduler, sounding, 1.0, true, &voices);
    check(voices.total() == 0, "both released");
    check(sounding.total() == 0, "and the MIDI pitch closed");
}

void testRetriggerOnlyReleasesTheSameInstrument()
{
    // A retrigger must release the previous note on that instrument, and only there.
    NoteScheduler scheduler;
    setUp(scheduler, {makeNote(0.1, 5.0, 60, BASS), makeNote(0.1, 0.2, 60, PIANO), makeNote(0.2, 0.4, 60, PIANO)});
    SoundingNotes sounding;
    SoundingVoices voices;

    runFor(scheduler, sounding, 0.3, true, &voices);
    check(voices.countFor(PIANO) == 1, "one piano note across the retrigger");
    check(voices.countFor(BASS) == 1, "the bass note is untouched by the piano retriggering");

    scheduler.stopAllNotes();
    runFor(scheduler, sounding, BLOCK_SECONDS, false, &voices);
    check(voices.total() == 0, "everything released");
    check(sounding.total() == 0, "on both outputs");
}

void testSwapReAnchorsPerInstrument()
{
    // Re-anchoring after a list swap looks up the note covering the playhead. Keyed on pitch alone
    // it could match the other instrument's note and keep a dropped one sounding.
    NoteScheduler scheduler;
    setUp(scheduler, {makeNote(0.0, 10.0, 60, PIANO), makeNote(0.0, 10.0, 60, BASS)});
    SoundingNotes sounding;
    SoundingVoices voices;

    runFor(scheduler, sounding, 0.5, true, &voices);
    check(voices.total() == 2, "both sounding before the swap");

    // The bass note is gone from the new list; the piano's is not.
    std::vector<NoteEvent> without_bass = {makeNote(0.0, 10.0, 60, PIANO)};
    scheduler.setNotes(without_bass);

    runFor(scheduler, sounding, 0.1, true, &voices);
    check(voices.countFor(BASS) == 0, "the dropped instrument's note stopped");
    check(voices.countFor(PIANO) == 1, "the surviving one kept playing");
    check(sounding.total() == 1, "and the pitch stays open on the wire, because one voice still has it");

    scheduler.stopAllNotes();
    runFor(scheduler, sounding, BLOCK_SECONDS, false, &voices);
    check(voices.total() == 0, "everything released at the end");
    check(sounding.total() == 0, "on both outputs");
}

void testStopReleasesEveryInstrument()
{
    NoteScheduler scheduler;
    setUp(scheduler, {makeNote(0.1, 30.0, 60, PIANO), makeNote(0.1, 30.0, 40, BASS), makeNote(0.1, 0.11, 36, DRUMS)});
    SoundingNotes sounding;
    SoundingVoices voices;

    runFor(scheduler, sounding, 0.5, true, &voices);
    check(voices.total() == 2, "the two sustained instruments are sounding, the drum hit is over");

    scheduler.stopAllNotes();
    runFor(scheduler, sounding, BLOCK_SECONDS, false, &voices);
    check(voices.total() == 0, "a stop releases every instrument");
    check(sounding.total() == 0, "and the MIDI output too");
}

void testMergeKeepsInstrumentsApart()
{
    std::vector<NoteEvent> notes = {makeNote(0.0, 1.0, 60, PIANO), makeNote(0.5, 1.5, 60, BASS)};
    NoteEvent::mergeOverlappingNotesWithSamePitch(notes);
    check(notes.size() == 2, "overlapping notes of different instruments are not merged");

    notes = {makeNote(0.0, 1.0, 60, PIANO), makeNote(0.5, 1.5, 60, PIANO)};
    NoteEvent::mergeOverlappingNotesWithSamePitch(notes);
    check(notes.size() == 1, "overlapping notes of the same instrument still are");
    check(notes.front().endTime == 1.5, "and the merged note runs to the later end");
}

// The piano roll's note range. Two rules that can pull against each other: it must cover every
// transcribed note, and it must be long enough to fill the column it is drawn in. The second is
// what decides how far past the notes it opens, and getting it wrong either crops notes or leaves
// the keyboard floating in a gap.

void testDisplayRangeFillsTheComponentWhenEmpty()
{
    using namespace PianoRollRange;

    // The real geometry: an 11 px key in the ~528 px column the 800 px window leaves.
    const DisplayRange range = computeDisplayRange(0, 0, false, 528.0f, 11.0f);

    check(keyboardLength(range, 11.0f) >= 528.0f, "an empty roll still fills its column");
    check(range.lowNote % 12 == 0, "and starts on a C");
    check(range.highNote % 12 == 11, "and ends on a B");
    check(range.lowNote <= 12 && range.highNote >= 83, "covering at least the default C0-B5");
}

void testDisplayRangeCoversEveryNote()
{
    using namespace PianoRollRange;

    // A bass part: nothing above the second octave.
    const DisplayRange bass = computeDisplayRange(28, 55, true, 528.0f, 11.0f);

    check(bass.lowNote <= 28 && bass.highNote >= 55, "the range covers the notes it was given");
    check(keyboardLength(bass, 11.0f) >= 528.0f, "and still fills the column");

    // Backwards arguments are the caller's mistake, not a crop.
    const DisplayRange swapped = computeDisplayRange(55, 28, true, 528.0f, 11.0f);
    check(swapped == bass, "low and high given the wrong way round give the same range");
}

void testDisplayRangeIsNotCroppedWhenItCannotFill()
{
    using namespace PianoRollRange;

    // Taller than 128 keys can fill: the answer is everything, not a crop.
    const DisplayRange range = computeDisplayRange(60, 62, true, 5000.0f, 11.0f);

    check(range.lowNote == MIN_MIDI_NOTE, "an over-tall column opens down to the bottom of MIDI");
    check(range.highNote == MAX_MIDI_NOTE, "and up to the top");
}

void testDisplayRangeStaysWithinMidi()
{
    using namespace PianoRollRange;

    const DisplayRange low = computeDisplayRange(0, 2, true, 528.0f, 11.0f);
    check(low.lowNote == MIN_MIDI_NOTE, "notes at the bottom of MIDI do not push the range below it");
    check(low.highNote <= MAX_MIDI_NOTE, "and the top end stays in range");

    const DisplayRange high = computeDisplayRange(125, 127, true, 528.0f, 11.0f);
    check(high.highNote == MAX_MIDI_NOTE, "notes at the top do not push the range above it");
    check(high.lowNote >= MIN_MIDI_NOTE, "and the bottom end stays in range");
    check(high.lowNote <= 125 && high.highNote >= 127, "while still covering the notes");
}

void testDisplayRangeOnlyGrowsWhileTranscribing()
{
    using namespace PianoRollRange;

    const DisplayRange first = computeDisplayRange(48, 60, true, 528.0f, 11.0f);
    const DisplayRange second = computeDisplayRange(24, 30, true, 528.0f, 11.0f);
    const DisplayRange merged = unionOf(first, second);

    check(merged.lowNote <= std::min(first.lowNote, second.lowNote), "a later chunk cannot raise the floor");
    check(merged.highNote >= std::max(first.highNote, second.highNote), "or lower the ceiling");
}

void testDisplayRangeSurvivesAZeroSizedComponent()
{
    using namespace PianoRollRange;

    // Before the first resize the column has no height, and a key can have no width either.
    check(computeDisplayRange(0, 0, false, 0.0f, 11.0f).lowNote % 12 == 0, "a zero-height column gives a whole octave");
    check(computeDisplayRange(0, 0, false, 528.0f, 0.0f).lowNote % 12 == 0, "and so does a zero-width key");
}

// WaveformPeaks answers "what are the peaks over this slice" for every bar the waveform draws, at
// any zoom, from a pyramid rather than the samples. Erring wide is fine -- erring narrow drops a
// transient, which is what looks broken -- so these pin down that a query always contains the
// truth, and never by more than a bin.

std::vector<float> makeSignal(int inNumSamples, int inSeed)
{
    juce::Random random(inSeed);
    std::vector<float> samples(static_cast<size_t>(inNumSamples));

    for (int i = 0; i < inNumSamples; ++i) {
        // Mostly quiet with occasional isolated spikes, so a query that loses a bin loses a peak.
        const float spike = random.nextInt(97) == 0 ? 1.0f : 0.15f;
        samples[static_cast<size_t>(i)] = (random.nextFloat() * 2.0f - 1.0f) * spike;
    }

    return samples;
}

WaveformMinMax bruteForce(const std::vector<float>& inSamples, int64_t inStart, int64_t inEnd)
{
    const int64_t start = std::max<int64_t>(0, inStart);
    const int64_t end = std::min(inEnd, static_cast<int64_t>(inSamples.size()));

    WaveformMinMax result = WaveformMinMax::empty();

    for (int64_t i = start; i < end; ++i) {
        const float sample = inSamples[static_cast<size_t>(i)];
        result.unionWith({sample, sample});
    }

    return result;
}

void testPeaksCoverTheWholeSignal()
{
    const auto samples = makeSignal(200000, 1);
    const auto truth = bruteForce(samples, 0, static_cast<int64_t>(samples.size()));

    WaveformPeaks peaks;
    peaks.buildFrom(samples);

    const WaveformPeaks::Reader reader(peaks);
    const auto whole = reader.query(0, static_cast<int64_t>(samples.size()));

    check(reader.getNumSamples() == static_cast<int64_t>(samples.size()), "peaks span every sample given");
    check(juce::exactlyEqual(whole.min, truth.min), "the full range gives the true minimum");
    check(juce::exactlyEqual(whole.max, truth.max), "the full range gives the true maximum");
}

void testPeaksQueryNeverLosesAPeak()
{
    const auto samples = makeSignal(300000, 2);

    WaveformPeaks peaks;
    peaks.buildFrom(samples);

    const WaveformPeaks::Reader reader(peaks);
    juce::Random random(3);

    for (int i = 0; i < 2000; ++i) {
        const int64_t start = random.nextInt(static_cast<int>(samples.size()));
        // Span the whole range the UI asks for, from a bar at max zoom to one at minimum zoom.
        const int64_t span = 1 + random.nextInt(static_cast<int>(samples.size()) - static_cast<int>(start));
        const auto got = reader.query(start, start + span);
        const auto truth = bruteForce(samples, start, start + span);

        check(got.min <= truth.min, "a query never reports a minimum above the truth");
        check(got.max >= truth.max, "a query never reports a maximum below the truth");
    }
}

void testPeaksQueryStaysCloseToTheTruth()
{
    const auto samples = makeSignal(300000, 4);

    WaveformPeaks peaks;
    peaks.buildFrom(samples);

    const WaveformPeaks::Reader reader(peaks);
    juce::Random random(5);

    for (int i = 0; i < 2000; ++i) {
        const int64_t start = random.nextInt(static_cast<int>(samples.size()));
        const int64_t span = 1 + random.nextInt(static_cast<int>(samples.size()) - static_cast<int>(start));
        const auto got = reader.query(start, start + span);

        // Bins are included whole, and the level is chosen so a bin is at most span/MIN_BINS wide,
        // so the answer cannot reach further than one bin past either end.
        const int64_t slack =
            std::max<int64_t>(span / WaveformPeaks::MIN_BINS_PER_QUERY, WaveformPeaks::LEVEL0_BIN_SAMPLES);
        const auto widened = bruteForce(samples, start - slack, start + span + slack);

        check(got.min >= widened.min, "a query does not reach more than a bin below the range");
        check(got.max <= widened.max, "a query does not reach more than a bin above the range");
    }
}

void testPeaksAppendedInChunksMatchOneAppend()
{
    const auto samples = makeSignal(150000, 6);

    WaveformPeaks whole;
    whole.append(samples);

    // The awkward case is a chunk ending mid-bin: the bin it stopped part-way through has to be
    // unioned into by the next chunk, not overwritten, and every level above it recomputed.
    WaveformPeaks chunked;
    juce::Random random(7);

    for (int64_t offset = 0; offset < static_cast<int64_t>(samples.size());) {
        const int64_t remaining = static_cast<int64_t>(samples.size()) - offset;
        const int64_t chunk = std::min<int64_t>(remaining, 1 + random.nextInt(700));
        chunked.append(
            std::span<const float>(samples).subspan(static_cast<size_t>(offset), static_cast<size_t>(chunk)));
        offset += chunk;
    }

    check(whole.getNumSamples() == chunked.getNumSamples(), "chunked appends cover the same samples");

    const WaveformPeaks::Reader whole_reader(whole);
    const WaveformPeaks::Reader chunked_reader(chunked);

    for (int i = 0; i < 2000; ++i) {
        const int64_t start = random.nextInt(static_cast<int>(samples.size()));
        const int64_t span = 1 + random.nextInt(static_cast<int>(samples.size()) - static_cast<int>(start));
        const auto from_whole = whole_reader.query(start, start + span);
        const auto from_chunks = chunked_reader.query(start, start + span);

        check(juce::exactlyEqual(from_whole.min, from_chunks.min), "chunked appends give the same minimum");
        check(juce::exactlyEqual(from_whole.max, from_chunks.max), "chunked appends give the same maximum");
    }
}

void testPeaksAppendMatchesBuildFrom()
{
    const auto samples = makeSignal(150000, 8);

    WaveformPeaks built;
    built.buildFrom(samples);

    WaveformPeaks appended;
    appended.append(samples);

    const WaveformPeaks::Reader built_reader(built);
    const WaveformPeaks::Reader appended_reader(appended);
    juce::Random random(9);

    for (int i = 0; i < 1000; ++i) {
        const int64_t start = random.nextInt(static_cast<int>(samples.size()));
        // Past the raw-scan threshold at both ends, so both readers take the pyramid: buildFrom
        // keeps the samples and answers short queries exactly, which append cannot.
        const int64_t span = WaveformPeaks::RAW_SCAN_MAX_SAMPLES + 1
                             + random.nextInt(static_cast<int>(samples.size()) - static_cast<int>(start));
        const auto from_built = built_reader.query(start, start + span);
        const auto from_appended = appended_reader.query(start, start + span);

        check(juce::exactlyEqual(from_built.min, from_appended.min), "append builds the same pyramid as buildFrom");
        check(juce::exactlyEqual(from_built.max, from_appended.max), "append builds the same pyramid as buildFrom");
    }
}

void testPeaksAreExactWhenZoomedIn()
{
    const auto samples = makeSignal(50000, 10);

    WaveformPeaks peaks;
    peaks.buildFrom(samples);

    const WaveformPeaks::Reader reader(peaks);
    juce::Random random(11);

    // A bar at full zoom is around 128 samples, well inside the raw-scan threshold, so what the
    // waveform draws when zoomed all the way in is the signal itself rather than an approximation.
    for (int i = 0; i < 500; ++i) {
        const int64_t start = random.nextInt(static_cast<int>(samples.size()) - 200);
        const auto got = reader.query(start, start + 128);
        const auto truth = bruteForce(samples, start, start + 128);

        check(juce::exactlyEqual(got.min, truth.min), "a zoomed-in query is exact");
        check(juce::exactlyEqual(got.max, truth.max), "a zoomed-in query is exact");
    }
}

void testPeaksSurviveDegenerateInput()
{
    WaveformPeaks empty;
    check(empty.getNumSamples() == 0, "a fresh store holds nothing");
    check(WaveformPeaks::Reader(empty).query(0, 1000).isEmpty(), "querying nothing gives nothing");

    empty.buildFrom({});
    check(empty.getNumSamples() == 0, "building from nothing holds nothing");

    const std::vector<float> one {0.5f};
    WaveformPeaks single;
    single.buildFrom(one);

    {
        const WaveformPeaks::Reader reader(single);
        check(juce::exactlyEqual(reader.query(0, 1).max, 0.5f), "a single sample is its own peak");
        check(reader.query(5, 10).isEmpty(), "a range past the end gives nothing");
        check(reader.query(-10, 0).isEmpty(), "a range before the start gives nothing");
        check(juce::exactlyEqual(reader.query(-10, 900).max, 0.5f), "an over-wide range is clamped");
    }

    // Shorter than one bin, so level 0 never fills and there is no level above it.
    const auto few = makeSignal(20, 12);
    WaveformPeaks partial;
    partial.buildFrom(few);

    const auto truth = bruteForce(few, 0, 20);
    const auto got = WaveformPeaks::Reader(partial).query(0, 20);
    check(juce::exactlyEqual(got.min, truth.min), "a signal shorter than one bin still reads back");
    check(juce::exactlyEqual(got.max, truth.max), "a signal shorter than one bin still reads back");

    single.clear();
    check(single.getNumSamples() == 0, "clearing drops everything");
}

// The waveform's bars are anchored to absolute content pixels so that scrolling reveals them
// rather than re-slicing them. These pin down that the slices tile the audio without gaps at every
// zoom the UI offers, and that where a bar starts does not depend on where the view is.

constexpr double BAR_PITCH = 4.0;
constexpr double WAVE_SAMPLE_RATE = 16000.0;

void testBarsTileTheAudioWithoutGaps()
{
    using namespace WaveformBars;

    // 0.1 and 5.0 are the zoom limits, against a base of 100 px/s.
    for (const double pixels_per_second: {10.0, 37.5, 100.0, 213.0, 500.0}) {
        int64_t previous = barStartSample(0, BAR_PITCH, pixels_per_second, WAVE_SAMPLE_RATE);
        check(previous == 0, "the first bar starts at the first sample");

        for (int64_t bar = 1; bar < 20000; ++bar) {
            const int64_t start = barStartSample(bar, BAR_PITCH, pixels_per_second, WAVE_SAMPLE_RATE);

            // A bar covers [start(i), start(i + 1)), so the slices tile the audio as long as the
            // starts advance. Going backwards would make a bar cover nothing and drop its audio.
            check(start >= previous, "bar starts never go backwards");
            previous = start;
        }
    }
}

void testBarStartsDoNotDependOnScroll()
{
    using namespace WaveformBars;

    const int64_t num_samples = 4 * 60 * static_cast<int64_t>(WAVE_SAMPLE_RATE);
    const auto whole = visibleBars(0.0f, 24000.0f, BAR_PITCH, num_samples, 100.0, WAVE_SAMPLE_RATE);

    // Scrolling by a pixel at a time must not shift which bar a given content pixel belongs to.
    // If it did -- if bars were laid out from the left edge of the view -- every bar would cover
    // slightly different audio each frame and the waveform would crawl.
    for (const float scroll: {0.0f, 1.0f, 2.0f, 3.0f, 971.0f, 12345.0f}) {
        const auto view = visibleBars(scroll, scroll + 972.0f, BAR_PITCH, num_samples, 100.0, WAVE_SAMPLE_RATE);

        check(!view.isEmpty(), "a scrolled view still has bars in it");
        check(view.firstBar == static_cast<int64_t>(std::floor(scroll / BAR_PITCH)),
              "the leftmost bar is the one the scroll position falls inside");
        check(view.firstBar >= whole.firstBar && view.lastBar <= whole.lastBar,
              "a scrolled view is a window onto the same bars, not a new set of them");
    }
}

void testVisibleBarsCoverTheClipAndStopAtTheAudio()
{
    using namespace WaveformBars;

    const int64_t num_samples = 10 * static_cast<int64_t>(WAVE_SAMPLE_RATE);

    const auto range = visibleBars(100.0f, 200.0f, BAR_PITCH, num_samples, 100.0, WAVE_SAMPLE_RATE);
    check(range.firstBar == 25, "the first bar is the one the clip starts inside");
    check(static_cast<double>(range.lastBar) * BAR_PITCH < 200.0, "bars past the clip are not drawn");
    check(static_cast<double>(range.lastBar + 1) * BAR_PITCH >= 200.0, "the clip is covered to its right edge");

    // Ten seconds at 100 px/s is 1000 px, so the audio runs out at bar 250.
    const auto past_end = visibleBars(0.0f, 4000.0f, BAR_PITCH, num_samples, 100.0, WAVE_SAMPLE_RATE);
    check(barStartSample(past_end.lastBar, BAR_PITCH, 100.0, WAVE_SAMPLE_RATE) < num_samples,
          "the last bar drawn has audio in it");
    check(barStartSample(past_end.lastBar + 1, BAR_PITCH, 100.0, WAVE_SAMPLE_RATE) >= num_samples,
          "and the one after it does not");

    check(visibleBars(0.0f, 972.0f, BAR_PITCH, 0, 100.0, WAVE_SAMPLE_RATE).isEmpty(), "no audio, no bars");
    check(visibleBars(500.0f, 100.0f, BAR_PITCH, num_samples, 100.0, WAVE_SAMPLE_RATE).isEmpty(),
          "an inverted clip gives nothing");
    check(visibleBars(0.0f, 972.0f, BAR_PITCH, num_samples, 0.0, WAVE_SAMPLE_RATE).isEmpty(),
          "a zero zoom gives nothing rather than dividing by it");
}

} // namespace

int main()
{
    testNotesStartAndStop();
    testShortNoteDoesNotSpanTwoBlocks();
    testSwapKeepsHeldNoteAndStopsRemovedOne();
    testSwapToEmptyListStopsEverything();
    testStopAndSeekRelease();
    testResetReleases();
    testRetriggerReleasesFirst();
    testPolyphonyCeilingStealsRatherThanLeaks();
    testStreamingSwapsNeverLeaveANoteSounding();

    testTwoInstrumentsShareAPitch();
    testRetriggerOnlyReleasesTheSameInstrument();
    testSwapReAnchorsPerInstrument();
    testStopReleasesEveryInstrument();
    testMergeKeepsInstrumentsApart();

    testDisplayRangeFillsTheComponentWhenEmpty();
    testDisplayRangeCoversEveryNote();
    testDisplayRangeIsNotCroppedWhenItCannotFill();
    testDisplayRangeStaysWithinMidi();
    testDisplayRangeOnlyGrowsWhileTranscribing();
    testDisplayRangeSurvivesAZeroSizedComponent();

    testPeaksCoverTheWholeSignal();
    testPeaksQueryNeverLosesAPeak();
    testPeaksQueryStaysCloseToTheTruth();
    testPeaksAppendedInChunksMatchOneAppend();
    testPeaksAppendMatchesBuildFrom();
    testPeaksAreExactWhenZoomedIn();
    testPeaksSurviveDegenerateInput();

    testBarsTileTheAudioWithoutGaps();
    testBarStartsDoNotDependOnScroll();
    testVisibleBarsCoverTheClipAndStopAtTheAudio();

    if (gFailures > 0) {
        std::printf("%d check(s) failed\n", gFailures);
        return 1;
    }

    std::printf("All checks passed\n");
    return 0;
}
