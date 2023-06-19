//
// Created by Damien Ronssin on 09.03.23.
//

#include "AudioUtils.h"
namespace AudioUtils
{

bool loadAudioFile(const juce::File& inFile, AudioBuffer<float>& outBuffer, double& outSampleRate)
{
    // Register different audio formats
    AudioFormatManager audio_format_manager;
    audio_format_manager.registerFormat(new juce::WavAudioFormat, true);
    audio_format_manager.registerFormat(new juce::AiffAudioFormat, false);
    audio_format_manager.registerFormat(new juce::FlacAudioFormat, false);

    std::unique_ptr<juce::AudioFormatReader> format_reader;
    format_reader.reset(audio_format_manager.createReaderFor(inFile));

    // Verify format reader is not null
    if (!format_reader)
        return false;

    // Get properties of input audio
    outSampleRate = format_reader->sampleRate;
    int num_source_samples = static_cast<int>(format_reader->lengthInSamples);

    outBuffer.setSize(2, num_source_samples);

    // Read source file. If not successful, return false
    if (!format_reader->read(&outBuffer, 0, num_source_samples, 0, true, true))
        return false;

    return true;
}

void resampleBuffer(const AudioBuffer<float>& inBuffer,
                    AudioBuffer<float>& outBuffer,
                    double inSourceSampleRate,
                    double inTargetSampleRate)
{
    if (inSourceSampleRate == inTargetSampleRate) {
        outBuffer.makeCopyOf(inBuffer);
        return;
    }

    Resampler resampler;
    // Prepare resampler
    resampler.prepareToPlay(inSourceSampleRate, inBuffer.getNumSamples(), inTargetSampleRate);
    auto num_expected_samples_after_resample = resampler.getNumOutSamplesOnNextProcessBlock(inBuffer.getNumSamples());

    outBuffer.setSize(inBuffer.getNumChannels(), num_expected_samples_after_resample);

    for (int ch = 0; ch < inBuffer.getNumChannels(); ch++) {
        int num_samples_after_resample = resampler.processBlock(
            inBuffer.getReadPointer(ch), outBuffer.getWritePointer(ch), inBuffer.getNumSamples());
        jassertquiet(num_samples_after_resample == num_expected_samples_after_resample);
    }
}
} // namespace AudioUtils