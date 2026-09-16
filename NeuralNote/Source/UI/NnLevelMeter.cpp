//
// Created by Damien Ronssin on 14.08.26.
//

#include "NnLevelMeter.h"

#include <algorithm>

#include "MeterScale.h"
#include "NnLook.h"

namespace
{
// Slow enough to follow with the eye rather than reading as a flicker, quick enough that a mute
// reads as off: the whole range drains in a second and a half.
constexpr float RELEASE_DB_PER_SECOND = 24.0f;

juce::Colour colourFor(int inSegment, int inNumSegments)
{
    switch (MeterScale::bandForSegment(inSegment, inNumSegments)) {
        case MeterScale::Band::Hot:
            return nn::colours::meterHot;
        case MeterScale::Band::Mid:
            return nn::colours::meterMid;
        case MeterScale::Band::Low:
        default:
            return nn::colours::meterLow;
    }
}
} // namespace

NnLevelMeter::NnLevelMeter(int inNumSegments, int inGap)
    : mNumSegments(inNumSegments)
    , mGap(inGap)
    , mDisplayDb(MeterScale::METER_MIN_DB)
{
    // The strip's clicks, drags and tooltip belong to the strip; the meter is only ever looked at.
    setInterceptsMouseClicks(false, false);
}

void NnLevelMeter::setLevelDb(float inInstantDb, float inDtSeconds)
{
    // Clamped to the top of the scale: a level above it looks the same, but left in mDisplayDb it
    // would hold the meter fully lit for however long the release takes to walk back into range.
    const float instant_db = std::min(inInstantDb, MeterScale::METER_MAX_DB);

    if (instant_db >= mDisplayDb) {
        // Instant attack, so a hit reads on the frame it lands.
        mDisplayDb = instant_db;
    } else {
        mDisplayDb = std::max(instant_db, mDisplayDb - RELEASE_DB_PER_SECOND * inDtSeconds);
    }

    const int lit = MeterScale::litSegmentsFor(mDisplayDb, mNumSegments);

    if (lit != mLitSegments) {
        mLitSegments = lit;
        repaint();
    }
}

void NnLevelMeter::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto gap = static_cast<float>(mGap);

    // Float widths: the last segment's right edge then lands exactly on the width, where rounding
    // each one would accumulate into a ragged end. Neither meter divides into whole pixels.
    const float segment_width =
        (bounds.getWidth() - static_cast<float>(mNumSegments - 1) * gap) / static_cast<float>(mNumSegments);

    if (segment_width <= 0.0f) {
        return;
    }

    const juce::Colour unlit = findColour(unlitColourId);

    for (int k = 0; k < mNumSegments; k++) {
        const juce::Rectangle<float> segment {bounds.getX() + static_cast<float>(k) * (segment_width + gap),
                                              bounds.getY(),
                                              segment_width,
                                              bounds.getHeight()};

        g.setColour(k < mLitSegments ? colourFor(k, mNumSegments) : unlit);
        g.fillRoundedRectangle(segment, nn::metrics::meterCorner);
    }
}
