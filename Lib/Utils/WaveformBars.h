//
// Created by Damien Ronssin on 14.08.26.
//

#ifndef WaveformBars_h
#define WaveformBars_h

#include <algorithm>
#include <cmath>
#include <cstdint>

/**
 * Where the waveform's bars fall along the timeline.
 *
 * Bars are anchored to absolute content pixels rather than to the visible window, so scrolling
 * never re-slices them: a bar covers the same audio wherever it happens to be on screen, and the
 * waveform reveals rather than crawls. Every bar covers its whole pitch, gap included, so no
 * sample goes unrepresented.
 */
namespace WaveformBars
{
/** The first sample of a bar. Bar i covers [barStartSample(i), barStartSample(i + 1)). */
inline int64_t barStartSample(int64_t inBar, double inBarPitchPx, double inPixelsPerSecond, double inSampleRate)
{
    if (inPixelsPerSecond <= 0.0 || inBar <= 0) {
        return 0;
    }

    // From the index rather than by accumulating a per-bar sample count: that count is fractional
    // at most zoom levels, and rounding it per bar would leave gaps that drop audio.
    return static_cast<int64_t>(static_cast<double>(inBar) * inBarPitchPx * inSampleRate / inPixelsPerSecond);
}

/** A run of bars, inclusive at both ends. */
struct BarRange {
    int64_t firstBar = 0;
    int64_t lastBar = -1;

    bool isEmpty() const { return lastBar < firstBar; }
    int64_t count() const { return isEmpty() ? 0 : lastBar - firstBar + 1; }
};

/**
 * The bars overlapping [inFromX, inToX) in content pixels, limited to those with audio in them.
 *
 * @param inNumSamples How much audio there is. Bars past it are dropped rather than drawn empty.
 */
inline BarRange visibleBars(float inFromX,
                            float inToX,
                            double inBarPitchPx,
                            int64_t inNumSamples,
                            double inPixelsPerSecond,
                            double inSampleRate)
{
    if (inBarPitchPx <= 0.0 || inPixelsPerSecond <= 0.0 || inNumSamples <= 0 || inToX <= inFromX) {
        return {};
    }

    const int64_t first = std::max<int64_t>(0, static_cast<int64_t>(std::floor(inFromX / inBarPitchPx)));
    const int64_t last = static_cast<int64_t>(std::ceil(inToX / inBarPitchPx)) - 1;

    if (last < first) {
        return {};
    }

    // The last bar that starts inside the audio. Walked back rather than trusted directly, since it
    // has to agree exactly with barStartSample and that is a floating-point division.
    auto highest = static_cast<int64_t>(static_cast<double>(inNumSamples) * inPixelsPerSecond
                                        / (inSampleRate * inBarPitchPx));

    while (highest > 0 && barStartSample(highest, inBarPitchPx, inPixelsPerSecond, inSampleRate) >= inNumSamples) {
        --highest;
    }

    while (barStartSample(highest + 1, inBarPitchPx, inPixelsPerSecond, inSampleRate) < inNumSamples) {
        ++highest;
    }

    return {first, std::min(last, highest)};
}
} // namespace WaveformBars

#endif // WaveformBars_h
