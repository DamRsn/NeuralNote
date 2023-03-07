//
// Created by Damien Ronssin on 05.03.23.
//

#include "DownSampler.h"

void DownSampler::prepareToPlay(double inSampleRate, int inMaxBlockSize)
{
    mSourceSampleRate = inSampleRate;
    mSpeedRatio = mSourceSampleRate / mTargetSampleRate;

    mInternalBuffer.setSize(
        1,
        inMaxBlockSize + 2 * static_cast<int>(LagrangeInterpolator::getBaseLatency())
            + 1);

    // Lowpass filter stuffs
    auto filter_coeffs =
        juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(
            static_cast<float>(mTargetSampleRate / 2.0), mSourceSampleRate, 4);

    mLowpassFilters.resize(static_cast<size_t>(filter_coeffs.size()));

    for (size_t i = 0; i < mLowpassFilters.size(); i++)
    {
        mLowpassFilters[i].coefficients = filter_coeffs[(size_t) i];
    }

    reset();
}

void DownSampler::reset()
{
    mInternalBuffer.clear();
    mNumInputSamplesAvailable = mInitPadding;
    mInterpolator.reset();

    for (auto& lowpass_filter: mLowpassFilters)
        lowpass_filter.reset();
}

int DownSampler::processBlock(const AudioBuffer<float>& inBuffer,
                              float* outBuffer,
                              int inNumSamples)
{
    jassert(mNumInputSamplesAvailable + inNumSamples <= mInternalBuffer.getNumSamples());

    mInternalBuffer.copyFrom(0, mNumInputSamplesAvailable, inBuffer, 0, 0, inNumSamples);
    float* internal_buffer_ptr = mInternalBuffer.getWritePointer(0);

    // Lowpass filter and copy data in internal buffer at the same time
    for (int i = 0; i < inNumSamples; i++)
    {
        for (auto& lowpass_filter: mLowpassFilters)
        {
            internal_buffer_ptr[mNumInputSamplesAvailable + i] =
                lowpass_filter.processSample(
                    internal_buffer_ptr[mNumInputSamplesAvailable + i]);
        }
    }

    mNumInputSamplesAvailable += inNumSamples;

    int num_out_samples_to_produce =
        static_cast<int>(std::floor(mNumInputSamplesAvailable / mSpeedRatio));

    int num_input_samples_used = mInterpolator.process(mSpeedRatio,
                                                       mInternalBuffer.getReadPointer(0),
                                                       outBuffer,
                                                       num_out_samples_to_produce);

    jassert(num_input_samples_used <= mNumInputSamplesAvailable);

    mNumInputSamplesAvailable -= num_input_samples_used;

    return num_out_samples_to_produce;
}

int DownSampler::numOutSamplesOnNextProcessBlock(int inNumSamples)
{
    return static_cast<int>(
        std::floor(mNumInputSamplesAvailable + inNumSamples / mSpeedRatio));
}
