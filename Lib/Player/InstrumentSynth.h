//
// Created by Damien Ronssin on 06.08.26.
//

#ifndef InstrumentSynth_h
#define InstrumentSynth_h

#include <array>
#include <atomic>
#include <memory>
#include <span>
#include <vector>

#include <JuceHeader.h>

#include "NoteEvent.h"
#include "RmsMeter.h"
#include "SynthEvent.h"

struct tsf;

/**
 * Plays a transcription on a General MIDI soundfont, one instrument at a time, with a per-instrument
 * fader.
 *
 * Each instrument gets its own tsf instance. tsf_copy shares the loaded sample data by refcount, so
 * that costs a voice pool rather than another copy of the soundfont, and the total rendering work is
 * unchanged because an instance only ever iterates its own voices. What it buys is a gain that can be
 * ramped: tsf's own per-channel volume is an instant dB step, so touching a fader or a mute under a
 * sounding note would click. It also means per-instrument metering, effects and stem export are
 * additions later rather than a rewrite.
 *
 * Because an instance serves exactly one instrument, tsf channels are not used at all -- the preset
 * is resolved once and notes are triggered by preset index. That also keeps note-on off tsf's
 * channel-allocation path.
 *
 * Threads: processBlock is the audio thread's. Everything else is the message thread's, and
 * ensureInstrument in particular must be called with the audio callback held off, because it
 * allocates and appends to the list processBlock walks.
 */
class InstrumentSynth
{
public:
    InstrumentSynth();

    ~InstrumentSynth();

    void prepareToPlay(double inSampleRate, int inSamplesPerBlock);

    /**
     * Creates the tsf instance for inProgram if there is not one already. Message thread, with the
     * audio callback held off. A no-op for an instrument that already exists, and while the
     * soundfont is still loading -- the caller does not have to sequence itself against that.
     *
     * @param inProgram 0-127, or msl::DRUM_PROGRAM.
     */
    void ensureInstrument(int inProgram);

    /** Drops every instrument. Message thread, with the audio callback held off. */
    void reset();

    /**
     * Renders inEvents and sums the result into ioBuffer. Audio thread only.
     *
     * @param ioBuffer Mono or stereo; the soundfont's own panning survives into a stereo one.
     * @param inEvents This block's events, in nondecreasing sample offset.
     * @param inNumSamples Block length.
     * @param inGain Master gain for the whole synth, ramped from the previous block's value.
     */
    void processBlock(AudioBuffer<float>& ioBuffer,
                      std::span<const SynthEvent> inEvents,
                      int inNumSamples,
                      float inGain);

    /** Releases every sounding voice on every instrument, drums included. Audio thread only. */
    void allNotesOff();

    /** @return Whether the soundfont has finished loading. Until then this renders silence. */
    bool isReady() const { return mFontReady.load(std::memory_order_acquire); }

    // Per-instrument parameters. Message thread; the audio thread reads them once per block.
    void setGainDb(int inProgram, float inGainDb);
    void setMuted(int inProgram, bool inMuted);
    void setSoloed(int inProgram, bool inSoloed);

    float getGainDb(int inProgram) const;
    bool isMuted(int inProgram) const;
    bool isSoloed(int inProgram) const;

    /**
     * @return The instrument's post-fader level as a mean square over the last window, published
     *         once per block by the audio thread. Zero for a program with no instrument. Message
     *         thread; convert with std::sqrt and Decibels::gainToDecibels.
     */
    float getMeterMeanSquare(int inProgram) const;

    // Per instrument, not in total. Reached by stealing the quietest voice, which is tsf's own
    // policy once the pool is pinned.
    static constexpr int MAX_VOICES_PER_INSTRUMENT = 32;

private:
    /** One instrument's player. Owns its tsf instance. */
    struct Instrument {
        tsf* font = nullptr;
        int program = 0;
        int presetIndex = -1;
        SmoothedValue<float, ValueSmoothingTypes::Linear> gain;

        // Post-fader, so a mute or a solo takes the meter down with the signal rather than needing
        // to be accounted for again on the way to the UI.
        RmsMeter meter;

        // Keys this preset plays with a looping sample, which therefore only stop on a note-off.
        // Read on the audio thread; written once, when the instrument is created.
        std::array<bool, 128> loopingKeys {};

        ~Instrument();

        Instrument() = default;
        Instrument(const Instrument&) = delete;
        Instrument& operator=(const Instrument&) = delete;
    };

    /**
     * Written by the message thread, read by the audio thread. Relaxed throughout: the three fields
     * of one instrument can be observed mid-update, and the worst case is a single block rendered
     * with a stale mute against a fresh gain, which is inaudible.
     */
    struct AtomicInstrumentParams {
        std::atomic<float> gainDb {0.0f};
        std::atomic<bool> muted {false};
        std::atomic<bool> soloed {false};
    };

    /** The audio thread's copy of the above, taken once per block. */
    struct InstrumentParams {
        float gainDb = 0.0f;
        bool muted = false;
        bool soloed = false;
    };

    void _loadFont();

    /** Reads the parameters and sets each instrument's gain target for this block. */
    void _updateGainTargets();

    /** Renders inNumSamples of every audible instrument into mMix at inStartSample. */
    void _renderSpan(int inStartSample, int inNumSamples);

    /** Copies each instrument's window into mMeterMeanSquare. End of the block, audio thread. */
    void _publishMeters();

    void _handleEvent(const SynthEvent& inEvent);

    // The soundfont every instrument's instance is a copy of. Never used to render.
    tsf* mFont = nullptr;

    // Release-stored once mFont is fully built, so the audio thread cannot see a half-loaded font.
    std::atomic<bool> mFontReady {false};

    ThreadPool mLoadPool {1};

    std::vector<std::unique_ptr<Instrument>> mInstruments;

    // Program to index in mInstruments, or -1. Saves a search per event on the audio thread.
    std::array<int, NUM_INSTRUMENT_IDS> mIndexForProgram;

    std::array<AtomicInstrumentParams, NUM_INSTRUMENT_IDS> mParams;
    std::array<InstrumentParams, NUM_INSTRUMENT_IDS> mSnapshot {};

    // The levels mParams' owner reads back. Indexed by program rather than by position in
    // mInstruments, so the UI never has to look at the vector the audio thread walks.
    std::array<std::atomic<float>, NUM_INSTRUMENT_IDS> mMeterMeanSquare {};

    static_assert(std::atomic<float>::is_always_lock_free);

    // One instrument's contribution, laid out as tsf writes it: the whole left channel then the
    // whole right, each as long as the span being rendered.
    std::vector<float> mScratch;

    // Every instrument summed, before the master gain. Separate from the caller's buffer so the
    // master ramp can be applied once over the whole block rather than per instrument.
    AudioBuffer<float> mMix;

    SmoothedValue<float, ValueSmoothingTypes::Linear> mMasterGain;

    double mSampleRate = 44100.0;
};

#endif // InstrumentSynth_h
