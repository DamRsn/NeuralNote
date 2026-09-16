//
// Created by Damien Ronssin on 06.08.26.
//

#include "InstrumentSynth.h"

#include <algorithm>

#include "GainConstants.h"
#include "SoundFontData.h"
#include "tsf.h"
#include "tsf_extras.h"

namespace
{
// How long a gain, mute or solo change takes to reach its new value. Long enough that a step under a
// sounding note is a fade rather than a click, short enough that a mute still feels immediate.
constexpr double GAIN_RAMP_SECONDS = 0.01;

// GM percussion lives in bank 128, and preset 0 is the Standard kit. tsf's own fallback chain in
// tsf_channel_set_presetnumber uses the same convention.
constexpr int DRUM_BANK = 128;
constexpr int DRUM_PRESET = 0;
} // namespace

InstrumentSynth::Instrument::~Instrument()
{
    // Decrements the shared refcount rather than freeing the samples, unless this was the last user.
    tsf_close(font);
}

InstrumentSynth::InstrumentSynth()
{
    mIndexForProgram.fill(-1);
    mInstruments.reserve(NUM_INSTRUMENT_IDS);

    // Off the message thread: ~40 MB of Vorbis to decode, over a second on an M1 Pro, and doing it
    // in the constructor would stall a DAW's plugin scan. Nothing can play until a transcription
    // exists, which takes minutes, so this is always finished long before it is needed -- and if it
    // somehow is not, the audio thread renders silence rather than racing.
    mLoadPool.addJob([this] { _loadFont(); });
}

InstrumentSynth::~InstrumentSynth()
{
    // Before mFont is freed: the job is still writing it.
    const bool finished = mLoadPool.removeAllJobs(true, 30000);
    jassertquiet(finished);

    mInstruments.clear();

    tsf_close(mFont);
}

void InstrumentSynth::_loadFont()
{
    tsf* font = tsf_load_memory(SoundFontData::MuseScore_General_sf3, SoundFontData::MuseScore_General_sf3Size);

    if (font == nullptr) {
        // tsf reports nothing more specific -- it does not even set an error code on the
        // no-sample-data path. In practice the one way this fails with a soundfont baked into the
        // binary is tsf having been built without its Ogg support, which tsf_impl.cpp makes a
        // compile error, so reaching here means something genuinely unexpected.
        Logger::writeToLog("InstrumentSynth: could not load the soundfont; playback will be silent");
        return;
    }

    mFont = font;
    mFontReady.store(true, std::memory_order_release);
}

void InstrumentSynth::prepareToPlay(double inSampleRate, int inSamplesPerBlock)
{
    mSampleRate = inSampleRate;

    mScratch.assign(static_cast<std::size_t>(2 * inSamplesPerBlock), 0.0f);
    mMix.setSize(2, inSamplesPerBlock);
    mMix.clear();

    mMasterGain.reset(inSampleRate, GAIN_RAMP_SECONDS);

    for (auto& instrument: mInstruments) {
        tsf_set_output(instrument->font, TSF_STEREO_UNWEAVED, static_cast<int>(inSampleRate), 0.0f);
        instrument->gain.reset(inSampleRate, GAIN_RAMP_SECONDS);
        instrument->meter.prepare(inSampleRate, METER_WINDOW_SECONDS);
    }

    for (auto& mean_square: mMeterMeanSquare) {
        mean_square.store(0.0f, std::memory_order_relaxed);
    }
}

void InstrumentSynth::ensureInstrument(int inProgram)
{
    jassert(inProgram >= 0 && inProgram < NUM_INSTRUMENT_IDS);

    if (!isReady() || mIndexForProgram[static_cast<std::size_t>(inProgram)] >= 0) {
        return;
    }

    const int preset_index = inProgram == msl::DRUM_PROGRAM ? tsf_get_presetindex(mFont, DRUM_BANK, DRUM_PRESET)
                                                            : tsf_get_presetindex(mFont, 0, inProgram);

    if (preset_index < 0) {
        // The bundled soundfont is General MIDI and covers every program the model can emit, so
        // this is a broken font rather than a runtime condition. Skipping leaves that instrument
        // silent instead of taking the rest of the transcription down with it.
        jassertfalse;
        Logger::writeToLog("InstrumentSynth: no preset for program " + String(inProgram));
        return;
    }

    auto instrument = std::make_unique<Instrument>();

    // Shares the sample data with mFont by refcount; only the voice and channel state is its own.
    instrument->font = tsf_copy(mFont);

    if (instrument->font == nullptr) {
        return;
    }

    instrument->program = inProgram;
    instrument->presetIndex = preset_index;

    tsfExtrasGetLoopingKeys(mFont, preset_index, instrument->loopingKeys.data());

    tsf_set_output(instrument->font, TSF_STEREO_UNWEAVED, static_cast<int>(mSampleRate), 0.0f);

    // Pins the voice pool. Without it tsf_note_on reallocs it four voices at a time -- on the audio
    // thread. With it, an exhausted pool steals the quietest voice instead.
    tsf_set_max_voices(instrument->font, MAX_VOICES_PER_INSTRUMENT);

    instrument->gain.reset(mSampleRate, GAIN_RAMP_SECONDS);
    instrument->gain.setCurrentAndTargetValue(Decibels::decibelsToGain(getGainDb(inProgram), MIN_INF_GAIN_DB));

    instrument->meter.prepare(mSampleRate, METER_WINDOW_SECONDS);
    mMeterMeanSquare[static_cast<std::size_t>(inProgram)].store(0.0f, std::memory_order_relaxed);

    mIndexForProgram[static_cast<std::size_t>(inProgram)] = static_cast<int>(mInstruments.size());
    mInstruments.push_back(std::move(instrument));
}

void InstrumentSynth::reset()
{
    mInstruments.clear();
    mIndexForProgram.fill(-1);

    for (auto& params: mParams) {
        params.gainDb.store(0.0f, std::memory_order_relaxed);
        params.muted.store(false, std::memory_order_relaxed);
        params.soloed.store(false, std::memory_order_relaxed);
    }

    // The instruments that would have republished these are gone, so a level left here would stay
    // lit for the rest of the session.
    for (auto& mean_square: mMeterMeanSquare) {
        mean_square.store(0.0f, std::memory_order_relaxed);
    }
}

void InstrumentSynth::processBlock(AudioBuffer<float>& ioBuffer,
                                   std::span<const SynthEvent> inEvents,
                                   int inNumSamples,
                                   float inGain)
{
    mMasterGain.setTargetValue(inGain);

    if (inNumSamples <= 0) {
        // No samples to advance over, so neither the ramp nor the meters' windows move. Unlike the
        // cases below, there is nothing to tell the meters: they are not holding a level they should
        // have walked down, no time passed.
        return;
    }

    if (mInstruments.empty()) {
        // Still has to advance, or the first block after an instrument appears jumps from wherever
        // the ramp was left to the current gain.
        mMasterGain.skip(inNumSamples);
        return;
    }

    if (inNumSamples > mMix.getNumSamples()) {
        // A block longer than prepareToPlay promised. Silence beats writing past the buffers.
        jassertfalse;
        mMasterGain.skip(inNumSamples);

        // Nothing was rendered, so the meters have to be told, or they hold their last level.
        for (auto& instrument: mInstruments) {
            instrument->meter.pushSilence(inNumSamples);
        }

        _publishMeters();
        return;
    }

    mMix.clear(0, inNumSamples);

    _updateGainTargets();

    int position = 0;

    for (const SynthEvent& event: inEvents) {
        const int offset = std::clamp(event.sampleOffset, 0, inNumSamples);

        if (offset > position) {
            _renderSpan(position, offset - position);
            position = offset;
        }

        _handleEvent(event);
    }

    if (position < inNumSamples) {
        _renderSpan(position, inNumSamples - position);
    }

    const float gain_at_start = mMasterGain.getCurrentValue();
    mMasterGain.skip(inNumSamples);
    const float gain_at_end = mMasterGain.getCurrentValue();

    for (int ch = 0; ch < mMix.getNumChannels(); ch++) {
        mMix.applyGainRamp(ch, 0, inNumSamples, gain_at_start, gain_at_end);
    }

    const int num_out = ioBuffer.getNumChannels();

    for (int ch = 0; ch < std::min(num_out, mMix.getNumChannels()); ch++) {
        ioBuffer.addFrom(ch, 0, mMix, ch, 0, inNumSamples);
    }

    if (num_out == 1) {
        // Fold rather than drop: the soundfont pans instruments across the stereo field, so taking
        // only the left channel would quietly lose whatever sits on the right.
        ioBuffer.addFrom(0, 0, mMix, 1, 0, inNumSamples);
    }

    _publishMeters();
}

void InstrumentSynth::_updateGainTargets()
{
    bool any_solo = false;

    for (const auto& instrument: mInstruments) {
        const auto program = static_cast<std::size_t>(instrument->program);
        const AtomicInstrumentParams& source = mParams[program];

        mSnapshot[program] = {source.gainDb.load(std::memory_order_relaxed),
                              source.muted.load(std::memory_order_relaxed),
                              source.soloed.load(std::memory_order_relaxed)};

        any_solo |= mSnapshot[program].soloed;
    }

    for (auto& instrument: mInstruments) {
        const InstrumentParams& params = mSnapshot[static_cast<std::size_t>(instrument->program)];

        // Solo is derived here rather than stored as "the others are muted": otherwise unsoloing
        // has to remember what was muted beforehand, and that memory drifts out of sync with the
        // mute state the moment both are touched.
        const bool audible = !params.muted && (!any_solo || params.soloed);

        instrument->gain.setTargetValue(audible ? Decibels::decibelsToGain(params.gainDb, MIN_INF_GAIN_DB) : 0.0f);
    }
}

void InstrumentSynth::_renderSpan(int inStartSample, int inNumSamples)
{
    float* const mix_left = mMix.getWritePointer(0);
    float* const mix_right = mMix.getWritePointer(1);

    for (auto& instrument: mInstruments) {
        // Nothing sounding and the fader at rest: rendering would cost a clear and a silent add.
        if (tsf_active_voice_count(instrument->font) == 0 && !instrument->gain.isSmoothing()) {
            instrument->gain.skip(inNumSamples);

            // Every instrument goes down exactly one of these two branches per span, so a meter
            // advances by the block length however the events split the block.
            instrument->meter.pushSilence(inNumSamples);
            continue;
        }

        // TSF_STEREO_UNWEAVED puts the whole left channel first, then the whole right.
        float* const left = mScratch.data();
        float* const right = left + inNumSamples;

        tsf_render_float(instrument->font, left, inNumSamples, /*flag_mixing=*/0);

        for (int i = 0; i < inNumSamples; i++) {
            const float gain = instrument->gain.getNextValue();
            const float l = left[i] * gain;
            const float r = right[i] * gain;

            mix_left[inStartSample + i] += l;
            mix_right[inStartSample + i] += r;

            // Folded to mono, so what the strip shows is what that strip contributes.
            instrument->meter.push((l + r) * 0.5f);
        }
    }
}

void InstrumentSynth::_publishMeters()
{
    for (const auto& instrument: mInstruments) {
        mMeterMeanSquare[static_cast<std::size_t>(instrument->program)].store(
            static_cast<float>(instrument->meter.getMeanSquare()), std::memory_order_relaxed);
    }
}

float InstrumentSynth::getMeterMeanSquare(int inProgram) const
{
    jassert(inProgram >= 0 && inProgram < NUM_INSTRUMENT_IDS);

    return mMeterMeanSquare[static_cast<std::size_t>(inProgram)].load(std::memory_order_relaxed);
}

void InstrumentSynth::_handleEvent(const SynthEvent& inEvent)
{
    if (inEvent.program < 0 || inEvent.program >= NUM_INSTRUMENT_IDS) {
        return;
    }

    if (inEvent.pitch < 0 || inEvent.pitch >= 128) {
        return;
    }

    const int index = mIndexForProgram[static_cast<std::size_t>(inEvent.program)];

    if (index < 0) {
        // A note for an instrument that has no instance yet -- the note list reached the audio
        // thread before ensureInstrument ran, or before the soundfont finished loading. It is
        // silent for a block or two rather than wrong.
        return;
    }

    Instrument& instrument = *mInstruments[static_cast<std::size_t>(index)];

    if (inEvent.isNoteOn) {
        tsf_note_on(instrument.font, instrument.presetIndex, inEvent.pitch, inEvent.velocity);
        return;
    }

    // General MIDI percussion is one-shot: the key selects a sample that is meant to ring out, and
    // the model gives a drum hit 10 ms of nominal duration. Honouring that note-off would start the
    // release envelope almost immediately and choke every cymbal. The scheduler still emits it --
    // its invariant and the MIDI output both need it -- and this is simply the consumer that
    // declines. allNotesOff is the path that does silence drums.
    //
    // Except on a key whose sample loops, which never ends on its own. Suppressing the note-off
    // there does not let it ring out, it sounds forever and never gives its voice back, and 32 of
    // those permanently silence the kit. In the bundled soundfont that is the open triangle and the
    // bell tree; releasing them still leaves 2.2 s and 18 s of sound, because the envelope release
    // is what actually ends them.
    const bool ends_on_its_own =
        inEvent.program == msl::DRUM_PROGRAM && !instrument.loopingKeys[static_cast<std::size_t>(inEvent.pitch)];

    if (!ends_on_its_own) {
        tsf_note_off(instrument.font, instrument.presetIndex, inEvent.pitch);
    }
}

void InstrumentSynth::allNotesOff()
{
    for (auto& instrument: mInstruments) {
        tsf_note_off_all(instrument->font);
    }
}

void InstrumentSynth::setGainDb(int inProgram, float inGainDb)
{
    jassert(inProgram >= 0 && inProgram < NUM_INSTRUMENT_IDS);
    mParams[static_cast<std::size_t>(inProgram)].gainDb.store(std::clamp(inGainDb, MIN_INF_GAIN_DB, MAX_GAIN_DB),
                                                              std::memory_order_relaxed);
}

void InstrumentSynth::setMuted(int inProgram, bool inMuted)
{
    jassert(inProgram >= 0 && inProgram < NUM_INSTRUMENT_IDS);
    mParams[static_cast<std::size_t>(inProgram)].muted.store(inMuted, std::memory_order_relaxed);
}

void InstrumentSynth::setSoloed(int inProgram, bool inSoloed)
{
    jassert(inProgram >= 0 && inProgram < NUM_INSTRUMENT_IDS);
    mParams[static_cast<std::size_t>(inProgram)].soloed.store(inSoloed, std::memory_order_relaxed);
}

float InstrumentSynth::getGainDb(int inProgram) const
{
    jassert(inProgram >= 0 && inProgram < NUM_INSTRUMENT_IDS);
    return mParams[static_cast<std::size_t>(inProgram)].gainDb.load(std::memory_order_relaxed);
}

bool InstrumentSynth::isMuted(int inProgram) const
{
    jassert(inProgram >= 0 && inProgram < NUM_INSTRUMENT_IDS);
    return mParams[static_cast<std::size_t>(inProgram)].muted.load(std::memory_order_relaxed);
}

bool InstrumentSynth::isSoloed(int inProgram) const
{
    jassert(inProgram >= 0 && inProgram < NUM_INSTRUMENT_IDS);
    return mParams[static_cast<std::size_t>(inProgram)].soloed.load(std::memory_order_relaxed);
}
