//
// Created by Damien Ronssin on 05.03.23.
//

#ifndef Resampler_h
#define Resampler_h

#include <JuceHeader.h>

class Resampler
{
public:
    Resampler() = default;

    ~Resampler() = default;

    void prepareToPlay(double inSourceSampleRate, int inMaxBlockSize, double inTargetSampleRate);

    void reset();

    int processBlock(const float* inBuffer, float* outBuffer, int inNumSamples);

    /**
     * Same, for a multichannel input that is averaged down to mono on the way in.
     *
     * The downmix happens here rather than in a separate pass because this already copies every
     * input sample into its own buffer to filter it: summing the channels at that point costs one
     * add per channel and saves a whole intermediate buffer. It also means the anti-alias filter
     * runs once on the signal the model actually gets, instead of once per channel on signals that
     * are then discarded.
     *
     * @param inChannels Per-channel read pointers.
     * @param inNumChannels How many, at least one.
     * @param outBuffer Mono output.
     * @param inNumSamples Input length, per channel.
     * @return How many output samples were produced.
     */
    int processBlock(const float* const* inChannels, int inNumChannels, float* outBuffer, int inNumSamples);

    int getNumOutSamplesOnNextProcessBlock(int inNumSamples) const;

private:
    LagrangeInterpolator mInterpolator;

    AudioBuffer<float> mInternalBuffer;

    const int mInitPadding = static_cast<int>(LagrangeInterpolator::getBaseLatency());

    int mNumInputSamplesAvailable = mInitPadding;
    double mSpeedRatio;
    double mSourceSampleRate;
    double mTargetSampleRate;

    // Low pass filter
    std::vector<juce::dsp::IIR::Filter<float>> mLowpassFilters;
};

#endif // Resampler_h
