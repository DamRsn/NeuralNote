#include "WhiteNoise.h"

namespace WhiteNoise
{
constexpr float gain = 0.5f;
constexpr size_t reservedNumSamples = 400000;

Oscillator::Oscillator()
{
    //Precache the samples to a vector
    samples.resize(reservedNumSamples);

    for (auto& sample: samples)
        sample = getNextSample() * gain;
}

void Oscillator::process(juce::AudioBuffer<float>& buffer) noexcept
{
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        auto nextSample = samples[(size_t)samplePos++];

        if (samplePos >= (int) samples.size())
            samplePos = 0;

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            buffer.setSample(channel, sample, nextSample);
    }
}

float Oscillator::getNextSample()
{
    auto f = rand.nextFloat();
    return juce::jmap(f, -1.f, 1.f);
}

} // namespace WhiteNoise
