//
// Created by Damien Ronssin on 14.08.26.
//

#ifndef RmsMeter_h
#define RmsMeter_h

#include <algorithm>
#include <cmath>
#include <vector>

/** The meters' integration time. Long enough to read as a level rather than as a waveform. */
static constexpr double METER_WINDOW_SECONDS = 0.05;

/**
 * A sliding mean square over the last window of samples, for driving a level meter.
 *
 * A circular buffer of squared samples plus a running sum: each sample subtracts the one leaving
 * the window and adds the one entering, so the cost per sample is constant however long the window
 * is, and the value is exact at every sample rather than once per block.
 *
 * Threading: prepare() and reset() belong to the message thread and must not run while the audio
 * callback can be in push() -- prepareToPlay is the only place either is safe. push() and
 * pushSilence() are the audio thread's and allocate nothing. Nothing here is atomic; the owner
 * publishes the result once per block into an atomic of its own, which is what the UI reads.
 */
class RmsMeter
{
public:
    /** Message thread, and only where the audio callback cannot be running. Allocates. */
    void prepare(double inSampleRate, double inWindowSeconds)
    {
        const auto size = static_cast<std::size_t>(std::max(1.0, std::round(inSampleRate * inWindowSeconds)));

        mSquares.assign(size, 0.0f);
        reset();
    }

    /** Same threading rules as prepare(). Does not resize. */
    void reset()
    {
        std::fill(mSquares.begin(), mSquares.end(), 0.0f);

        mSum = 0.0;
        mWrite = 0;

        // A fresh window is silent, so pushSilence has nothing to do until something is pushed.
        mZeroRun = static_cast<int>(mSquares.size());
    }

    /** Audio thread. One sample into the window. */
    void push(float inSample) noexcept
    {
        const auto size = static_cast<int>(mSquares.size());

        if (size == 0) {
            return;
        }

        const float square = inSample * inSample;
        auto& slot = mSquares[static_cast<std::size_t>(mWrite)];

        // The running sum is a double: a float one accumulates rounding across millions of
        // add/subtract pairs and drifts away from the window it is meant to describe.
        mSum += static_cast<double>(square) - static_cast<double>(slot);
        slot = square;

        if (++mWrite >= size) {
            mWrite = 0;
        }

        mZeroRun = 0;
    }

    /**
     * Audio thread. inNumSamples of silence, touching no memory once the window is already all
     * zeros -- which is what makes an instrument that is not sounding cost nothing.
     */
    void pushSilence(int inNumSamples) noexcept
    {
        const auto size = static_cast<int>(mSquares.size());

        if (size == 0 || inNumSamples <= 0 || mZeroRun >= size) {
            return;
        }

        // Past a full window every entry it could evict is already a zero, so the extra writes
        // would be a no-op and the write position they would leave behind does not matter.
        const int num = std::min(inNumSamples, size);

        for (int i = 0; i < num; i++) {
            auto& slot = mSquares[static_cast<std::size_t>(mWrite)];

            mSum -= static_cast<double>(slot);
            slot = 0.0f;

            if (++mWrite >= size) {
                mWrite = 0;
            }
        }

        mZeroRun = std::min(mZeroRun + inNumSamples, size);

        if (mZeroRun >= size) {
            // Exactly zero rather than whatever the subtractions rounded to, so silence reads as
            // silence and the residue is cleared every time the signal stops.
            mSum = 0.0;
        }
    }

    /** Never negative: rounding can leave the sum a hair below zero, and NaN dB spreads from there. */
    double getMeanSquare() const noexcept
    {
        if (mSquares.empty() || mSum <= 0.0) {
            return 0.0;
        }

        return mSum / static_cast<double>(mSquares.size());
    }

    int getWindowSamples() const noexcept { return static_cast<int>(mSquares.size()); }

private:
    std::vector<float> mSquares;
    double mSum = 0.0;
    int mWrite = 0;
    int mZeroRun = 0;
};

#endif // RmsMeter_h
