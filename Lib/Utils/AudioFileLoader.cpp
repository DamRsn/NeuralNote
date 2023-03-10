//
// Created by Damien Ronssin on 09.03.23.
//

#include "AudioFileLoader.h"
AudioFileLoader::AudioFileLoader()
{
    // Register different audio formats
    mAudioFormatManager.registerFormat(new juce::WavAudioFormat, true);
    mAudioFormatManager.registerFormat(new juce::AiffAudioFormat, false);
    mAudioFormatManager.registerFormat(new juce::FlacAudioFormat, false);

    // TODO: MP3 support? JUCE doc is scary
}

AudioFileLoader::~AudioFileLoader()
{
}

bool AudioFileLoader::loadAudioFile(const juce::File& inFile,
                                    AudioBuffer<float>& outBuffer,
                                    int& outNumOutSamples)
{
    std::unique_ptr<juce::AudioFormatReader> format_reader;
    format_reader.reset(mAudioFormatManager.createReaderFor(inFile));

    // Verify format reader is not null
    if (!format_reader)
        return false;

    // Get properties of input audio
    double source_sample_rate = format_reader->sampleRate;
    int num_source_samples = static_cast<int>(format_reader->lengthInSamples);

    // Prepare downsampler and check if number of samples is not to high.
    mDownSampler.prepareToPlay(source_sample_rate, num_source_samples);
    auto num_expected_down_samples =
        mDownSampler.numOutSamplesOnNextProcessBlock(num_source_samples);

    if (num_expected_down_samples > outBuffer.getNumSamples())
        return false;

    AudioBuffer<float> audio_buffer_source;
    audio_buffer_source.setSize(1, num_source_samples);

    // Read source file. If not successful, return false
    if (!format_reader->read(&audio_buffer_source, 0, num_source_samples, 0, true, false))
        return false;

    // Downsample and put result in provided buffer
    outNumOutSamples = mDownSampler.processBlock(
        audio_buffer_source, outBuffer.getWritePointer(0), num_source_samples);

    jassert(outNumOutSamples <= num_expected_down_samples);

    return true;
}
