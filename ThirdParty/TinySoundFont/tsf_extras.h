// Small queries about a loaded soundfont that tsf has no public API for. Their
// definitions live in tsf_impl.cpp, the one translation unit that can see tsf's
// internal structs -- tsf.h only declares them under TSF_IMPLEMENTATION.

#ifndef tsf_extras_h
#define tsf_extras_h

struct tsf;

/**
 * Reports which keys of a preset are played by a looping sample.
 *
 * A looping region only stops when its amplitude envelope does, which means it needs a note-off:
 * left held, it sounds forever and its voice is never returned to the pool. Most of a General MIDI
 * percussion kit is one-shot and wants no note-off, so a caller that suppresses them needs to know
 * the exceptions rather than assume there are none.
 *
 * @param inFont Loaded soundfont. Null is allowed and reports nothing.
 * @param inPresetIndex Preset to inspect, as returned by tsf_get_presetindex. Out-of-range reports
 *        nothing.
 * @param outKeys Filled for all 128 MIDI keys, true where at least one region of this preset loops.
 */
void tsfExtrasGetLoopingKeys(const tsf* inFont, int inPresetIndex, bool* outKeys);

#endif // tsf_extras_h
