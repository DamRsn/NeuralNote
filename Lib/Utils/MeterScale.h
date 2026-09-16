//
// Created by Damien Ronssin on 14.08.26.
//

#ifndef MeterScale_h
#define MeterScale_h

#include <algorithm>
#include <cmath>

/**
 * How a level becomes lit segments, and which band a segment belongs to.
 *
 * Pure arithmetic, kept out of the component so the strip's 16 segments and the master's 26 cannot
 * end up on different scales.
 */
namespace MeterScale
{
/**
 * What the meters show, which is not what the faders span: the gain range runs to +6 dB and its
 * floor stands for silence, whereas a meter needs a bottom it can actually reach and a top at
 * clipping. Deliberately its own pair of constants rather than the ones in GainConstants.h.
 */
inline constexpr float METER_MIN_DB = -36.0f;
inline constexpr float METER_MAX_DB = 0.0f;

/** Where a segment stops being green, and where it becomes hot. */
inline constexpr float METER_MID_DB = -12.0f;
inline constexpr float METER_HOT_DB = -6.0f;

inline constexpr int RANGE_DB = static_cast<int>(METER_MAX_DB - METER_MIN_DB);
inline constexpr int MID_ABOVE_FLOOR_DB = static_cast<int>(METER_MID_DB - METER_MIN_DB);
inline constexpr int HOT_ABOVE_FLOOR_DB = static_cast<int>(METER_HOT_DB - METER_MIN_DB);

static_assert(RANGE_DB == 36);
static_assert(MID_ABOVE_FLOOR_DB == 24 && HOT_ABOVE_FLOOR_DB == 30);

enum class Band { Low, Mid, Hot };

/**
 * The band a segment belongs to: a band starts at the segment whose dB span contains its threshold,
 * which the truncating division below is exactly the index of. Compared in integers so the boundary
 * is a segment index rather than a hair either side of a float threshold.
 *
 * Deriving it from dB rather than from the index is what keeps the two meter sizes on the same
 * scale: 0-16 / 17-20 / 21-25 at N = 26, 0-9 / 10-12 / 13-15 at N = 16. Not the same level to the
 * decibel, though, and it cannot be -- no segment boundary lands on -12 or -6 at either size, and
 * litSegmentsFor rounds, so the segment lights at the middle of its span. The master turns hot at
 * -6.23 dB and the strip at -5.63, each within half a segment of METER_HOT_DB.
 */
inline Band bandForSegment(int inSegment, int inNumSegments)
{
    if (inSegment >= HOT_ABOVE_FLOOR_DB * inNumSegments / RANGE_DB) {
        return Band::Hot;
    }

    if (inSegment >= MID_ABOVE_FLOOR_DB * inNumSegments / RANGE_DB) {
        return Band::Mid;
    }

    return Band::Low;
}

/** How many segments a level lights. Rounded, so a segment is drawn whole or not at all. */
inline int litSegmentsFor(float inDb, int inNumSegments)
{
    if (!std::isfinite(inDb)) {
        return 0;
    }

    const float normalised = (inDb - METER_MIN_DB) / static_cast<float>(RANGE_DB);
    const int lit = static_cast<int>(std::lround(normalised * static_cast<float>(inNumSegments)));

    return std::clamp(lit, 0, inNumSegments);
}
} // namespace MeterScale

#endif // MeterScale_h
