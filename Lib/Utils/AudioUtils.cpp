//
// Created by Damien Ronssin on 09.03.23.
//

#include "AudioUtils.h"

#define MINIMP3_IMPLEMENTATION
#include "minimp3.h"
#include "minimp3_ex.h"

namespace AudioUtils
{
bool loadAudioFile(const juce::File& inFile, AudioBuffer<float>& outBuffer, double& outSampleRate)
{
    if (inFile.getFileExtension() == ".mp3") {
        return _loadMP3File(inFile.getFullPathName().toStdString(), outBuffer, outSampleRate);
    }

    // Register different audio formats
    auto audio_format_manager = createAudioFormatManager();

    std::unique_ptr<juce::AudioFormatReader> format_reader;
    format_reader.reset(audio_format_manager->createReaderFor(inFile));

    // Verify format reader is not null
    if (!format_reader)
        return false;

    // Get properties of input audio
    outSampleRate = format_reader->sampleRate;
    int num_source_samples = static_cast<int>(format_reader->lengthInSamples);
    int num_channels = (int) format_reader->numChannels;

    outBuffer.setSize(num_channels, num_source_samples);

    // Read source file. If not successful, return false
    if (!format_reader->read(&outBuffer, 0, num_source_samples, 0, true, true))
        return false;

    return true;
}

StringArray getSupportedAudioFileExtensions()
{
    StringArray supported_extensions;
    supported_extensions.add(".mp3");

    auto audio_format_manager = createAudioFormatManager();
    for (auto& format: *audio_format_manager) {
        StringArray file_extensions = format->getFileExtensions();
        for (auto& extension: file_extensions) {
            supported_extensions.add(extension);
        }
    }

    return supported_extensions;
}

std::unique_ptr<AudioFormatManager> createAudioFormatManager()
{
    auto audio_format_manager = std::make_unique<AudioFormatManager>();
    audio_format_manager->registerFormat(new juce::WavAudioFormat, true);
    audio_format_manager->registerFormat(new juce::AiffAudioFormat, false);
    audio_format_manager->registerFormat(new juce::FlacAudioFormat, false);
    audio_format_manager->registerFormat(new juce::OggVorbisAudioFormat, false);

    return std::move(audio_format_manager);
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
        resampler.reset();
        int num_samples_after_resample = resampler.processBlock(
            inBuffer.getReadPointer(ch), outBuffer.getWritePointer(ch), inBuffer.getNumSamples());
        jassertquiet(num_samples_after_resample == num_expected_samples_after_resample);
    }
}

bool _loadMP3File(const std::string& filename, juce::AudioBuffer<float>& outBuffer, double& outSampleRate)
{
    mp3dec_t mp3d;
    mp3dec_file_info_t info;
    int loadResult = mp3dec_load(&mp3d, filename.c_str(), &info, nullptr, nullptr);

    if (loadResult) {
        return false;
    }

    outBuffer.setSize(info.channels, static_cast<int>(info.samples / info.channels));

    for (size_t i = 0; i < info.samples; ++i) {
        size_t channel = i % info.channels;
        outBuffer.setSample((int) channel, static_cast<int>(i / info.channels), (float) info.buffer[i] / 32768.0f);
    }

    outSampleRate = static_cast<double>(info.hz);
    free(info.buffer);
    return true;
}

} // namespace AudioUtils