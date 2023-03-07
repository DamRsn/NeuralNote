//
// Created by Damien Ronssin on 05.03.23.
//

#ifndef DownSampler_h
#define DownSampler_h

#include <JuceHeader.h>

class DownSampler
{
public:
    DownSampler() = default;

    ~DownSampler() = default;

    void prepareToPlay(double inSampleRate, int inMaxBlockSize);

    void reset();

    int processBlock(const juce::AudioBuffer<float>& inBuffer,
                     float* outBuffer,
                     int inNumSamples);

    int numOutSamplesOnNextProcessBlock(int inNumSamples);

private:
    LagrangeInterpolator mInterpolator;

    AudioBuffer<float> mInternalBuffer;

    const int mInitPadding = static_cast<int>(LagrangeInterpolator::getBaseLatency());

    int mNumInputSamplesAvailable = mInitPadding;
    double mSpeedRatio;
    double mSourceSampleRate;
    const double mTargetSampleRate = 22050.0;

    // Low pass filter
    std::vector<juce::dsp::IIR::Filter<float>> mLowpassFilters;
};

#endif // DownSampler_h
