//
// Created by Damien Ronssin on 14.08.26.
//

#ifndef WaveformPeaks_h
#define WaveformPeaks_h

#include <cstdint>
#include <span>
#include <vector>

#include <JuceHeader.h>

/** The vertical extent of one slice of audio. An empty slice has min > max. */
struct WaveformMinMax {
    float min = 0.0f;
    float max = 0.0f;

    bool isEmpty() const { return min > max; }

    void unionWith(const WaveformMinMax& inOther)
    {
        min = std::min(min, inOther.min);
        max = std::max(max, inOther.max);
    }

    static WaveformMinMax empty() { return {std::numeric_limits<float>::max(), std::numeric_limits<float>::lowest()}; }
};

/**
 * Min/max peaks over a mono signal, stored as a pyramid so a query costs the same however much
 * audio it spans.
 *
 * Level 0 holds one min/max per LEVEL0_BIN_SAMPLES; each level above combines pairs from the one
 * below. A query picks the coarsest level whose bins are small enough to keep the answer tight,
 * so a bar spanning ten minutes reads about as many bins as one spanning a second. Below
 * RAW_SCAN_MAX_SAMPLES it reads the samples themselves and is exact.
 *
 * Peaks are appended from the writer thread while recording and read from the message thread while
 * painting, so all access goes through Reader, which holds the lock for a whole frame rather than
 * per query -- 200 separate locks would be both slower and inconsistent, since the pyramid can
 * grow between them.
 */
class WaveformPeaks
{
public:
    /** 4 ms at 16 kHz. Finer than one bar at any zoom the UI offers. */
    static constexpr int64_t LEVEL0_BIN_SAMPLES = 64;

    /**
     * How many bins a query reads at the level it settles on. Bins are included whole, so a query
     * covers up to one extra bin at each end -- this bounds that overshoot to 1/8 of the range.
     */
    static constexpr int64_t MIN_BINS_PER_QUERY = 16;

    /**
     * Below this, a query scans the samples directly instead. The pyramid's bins are 64 samples,
     * which is too coarse to be tight once a bar is only a couple of hundred samples wide, and at
     * that point scanning is cheap anyway.
     */
    static constexpr int64_t RAW_SCAN_MAX_SAMPLES = 2048;

    /**
     * Replaces the contents with peaks over the whole of inSamples.
     *
     * The buffer is borrowed, not copied: it must outlive this object or be dropped with clear().
     * Keeping it is what lets queries be exact when zoomed in.
     */
    void buildFrom(std::span<const float> inSamples);

    /**
     * Extends the peaks, for a recording that is still growing. No samples are retained, so queries
     * fall back to the pyramid at every zoom.
     *
     * Appending in chunks gives the same pyramid as one buildFrom over the concatenation.
     */
    void append(std::span<const float> inSamples);

    void clear();

    int64_t getNumSamples() const;

    /** Holds the lock for as long as it lives, so every query it serves sees the same audio. */
    class Reader
    {
    public:
        explicit Reader(const WaveformPeaks& inPeaks)
            : mPeaks(inPeaks)
            , mLock(inPeaks.mLock)
        {
        }

        /** Peaks over [inStartSample, inEndSample), clamped to what exists. Empty if nothing does. */
        WaveformMinMax query(int64_t inStartSample, int64_t inEndSample) const;

        int64_t getNumSamples() const { return mPeaks.mNumSamples; }

    private:
        const WaveformPeaks& mPeaks;
        const juce::ScopedLock mLock;
    };

private:
    /** Recomputes levels above 0 for the bins covering [inFirstBin, inLastBin] at level 0. */
    void _cascade(int64_t inFirstBin, int64_t inLastBin);

    juce::CriticalSection mLock;

    /** mLevels[k] has bins of LEVEL0_BIN_SAMPLES << k. The top level is a single bin. */
    std::vector<std::vector<WaveformMinMax>> mLevels;

    /** The audio the peaks were built from, when it is still around. Empty while recording. */
    std::span<const float> mRawSamples;

    int64_t mNumSamples = 0;
};

#endif // WaveformPeaks_h
