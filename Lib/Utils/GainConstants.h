//
// Created by Damien Ronssin on 08.08.26.
//

#ifndef GainConstants_h
#define GainConstants_h

/**
 * The dB at which every gain in the plugin is silence, and the floor of every range that feeds
 * one: the master gain and the per-instrument faders.
 *
 * One constant rather than one per stage, so a fader at the bottom of its travel means the same
 * thing wherever it is drawn.
 */
static constexpr float MIN_INF_GAIN_DB = -36.0f;

/** The matching ceiling, for the same reason. */
static constexpr float MAX_GAIN_DB = 6.0f;

#endif // GainConstants_h
