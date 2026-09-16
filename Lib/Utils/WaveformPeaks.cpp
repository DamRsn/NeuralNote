//
// Created by Damien Ronssin on 14.08.26.
//

#include "WaveformPeaks.h"

namespace
{
/** Peaks over a run of samples, as one level-0 bin's worth. */
WaveformMinMax scan(std::span<const float> inSamples)
{
    if (inSamples.empty()) {
        return WaveformMinMax::empty();
    }

    const auto range = juce::FloatVectorOperations::findMinAndMax(inSamples.data(), static_cast<int>(inSamples.size()));
    return {range.getStart(), range.getEnd()};
}

/** [inStart, inEnd) of inSamples. Callers clamp inEnd to the span first. */
std::span<const float> slice(std::span<const float> inSamples, int64_t inStart, int64_t inEnd)
{
    return inSamples.subspan(static_cast<size_t>(inStart), static_cast<size_t>(inEnd - inStart));
}
} // namespace

void WaveformPeaks::buildFrom(std::span<const float> inSamples)
{
    const juce::ScopedLock sl(mLock);

    mLevels.clear();
    mRawSamples = inSamples;
    mNumSamples = static_cast<int64_t>(inSamples.size());

    if (mNumSamples == 0) {
        mRawSamples = {};
        return;
    }

    const int64_t num_bins = (mNumSamples + LEVEL0_BIN_SAMPLES - 1) / LEVEL0_BIN_SAMPLES;

    auto& level0 = mLevels.emplace_back(static_cast<size_t>(num_bins));

    for (int64_t bin = 0; bin < num_bins; ++bin) {
        const int64_t start = bin * LEVEL0_BIN_SAMPLES;
        level0[static_cast<size_t>(bin)] =
            scan(slice(inSamples, start, std::min(start + LEVEL0_BIN_SAMPLES, mNumSamples)));
    }

    // Every level above is pairs of the one below, up to a single bin covering everything.
    while (mLevels.back().size() > 1) {
        const auto& below = mLevels.back();
        std::vector<WaveformMinMax> level((below.size() + 1) / 2);

        for (size_t bin = 0; bin < level.size(); ++bin) {
            WaveformMinMax value = below[bin * 2];

            if (bin * 2 + 1 < below.size()) {
                value.unionWith(below[bin * 2 + 1]);
            }

            level[bin] = value;
        }

        mLevels.push_back(std::move(level));
    }
}

void WaveformPeaks::append(std::span<const float> inSamples)
{
    if (inSamples.empty()) {
        return;
    }

    const juce::ScopedLock sl(mLock);

    // A growing recording has no buffer to scan, so drop any borrowed one rather than let a query
    // read samples that no longer line up with the peaks.
    mRawSamples = {};

    if (mLevels.empty()) {
        mLevels.emplace_back();
    }

    const int64_t start = mNumSamples;
    const int64_t end = start + static_cast<int64_t>(inSamples.size());
    const int64_t first_bin = start / LEVEL0_BIN_SAMPLES;
    const int64_t last_bin = (end - 1) / LEVEL0_BIN_SAMPLES;

    auto& level0 = mLevels[0];

    if (level0.size() < static_cast<size_t>(last_bin + 1)) {
        level0.resize(static_cast<size_t>(last_bin + 1), WaveformMinMax::empty());
    }

    // The bin the previous append stopped part-way through is unioned into rather than overwritten,
    // which is what makes appending in chunks equivalent to one pass over the whole signal.
    for (int64_t bin = first_bin; bin <= last_bin; ++bin) {
        const int64_t bin_start = std::max(start, bin * LEVEL0_BIN_SAMPLES);
        const int64_t bin_end = std::min(end, (bin + 1) * LEVEL0_BIN_SAMPLES);

        level0[static_cast<size_t>(bin)].unionWith(scan(slice(inSamples, bin_start - start, bin_end - start)));
    }

    mNumSamples = end;

    _cascade(first_bin, last_bin);
}

void WaveformPeaks::_cascade(int64_t inFirstBin, int64_t inLastBin)
{
    int64_t first_bin = inFirstBin;
    int64_t last_bin = inLastBin;

    for (size_t level = 1; mLevels[level - 1].size() > 1; ++level) {
        if (mLevels.size() <= level) {
            mLevels.emplace_back();
        }

        first_bin /= 2;
        last_bin /= 2;

        const auto& below = mLevels[level - 1];
        auto& current = mLevels[level];

        if (current.size() < static_cast<size_t>(last_bin + 1)) {
            current.resize(static_cast<size_t>(last_bin + 1), WaveformMinMax::empty());
        }

        for (int64_t bin = first_bin; bin <= last_bin; ++bin) {
            WaveformMinMax value = below[static_cast<size_t>(bin) * 2];

            if (static_cast<size_t>(bin) * 2 + 1 < below.size()) {
                value.unionWith(below[static_cast<size_t>(bin) * 2 + 1]);
            }

            current[static_cast<size_t>(bin)] = value;
        }
    }
}

void WaveformPeaks::clear()
{
    const juce::ScopedLock sl(mLock);

    mLevels.clear();
    mRawSamples = {};
    mNumSamples = 0;
}

int64_t WaveformPeaks::getNumSamples() const
{
    const juce::ScopedLock sl(mLock);

    return mNumSamples;
}

WaveformMinMax WaveformPeaks::Reader::query(int64_t inStartSample, int64_t inEndSample) const
{
    const int64_t start = std::max<int64_t>(0, inStartSample);
    const int64_t end = std::min(inEndSample, mPeaks.mNumSamples);

    if (end <= start || mPeaks.mLevels.empty()) {
        return WaveformMinMax::empty();
    }

    const int64_t span = end - start;

    if (!mPeaks.mRawSamples.empty() && span <= RAW_SCAN_MAX_SAMPLES) {
        return scan(slice(mPeaks.mRawSamples, start, end));
    }

    // The coarsest level whose bins are still small enough that including them whole cannot widen
    // the answer by much.
    size_t level = 0;

    while (level + 1 < mPeaks.mLevels.size() && (LEVEL0_BIN_SAMPLES << (level + 1)) * MIN_BINS_PER_QUERY <= span) {
        ++level;
    }

    const int64_t bin_samples = LEVEL0_BIN_SAMPLES << level;
    const auto& bins = mPeaks.mLevels[level];

    // Floor at both ends, so every bin the range touches is included. Erring wide keeps a transient
    // that straddles a bin boundary; erring narrow would drop it, which is what looks broken.
    const int64_t first_bin = start / bin_samples;
    const int64_t last_bin = std::min<int64_t>((end - 1) / bin_samples, static_cast<int64_t>(bins.size()) - 1);

    WaveformMinMax result = WaveformMinMax::empty();

    for (int64_t bin = first_bin; bin <= last_bin; ++bin) {
        result.unionWith(bins[static_cast<size_t>(bin)]);
    }

    return result;
}
