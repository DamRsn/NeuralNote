//
// Created by Damien Ronssin on 09.03.23.
//

#include "AudioUtils.h"

// Declarations only: the implementation lives in the minimp3 target, so its
// warnings do not land on this file. See ThirdParty/minimp3/CMakeLists.txt.
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

    return audio_format_manager;
}

void resampleBuffer(const AudioBuffer<float>& inBuffer,
                    AudioBuffer<float>& outBuffer,
                    double inSourceSampleRate,
                    double inTargetSampleRate)
{
    if (juce::exactlyEqual(inSourceSampleRate, inTargetSampleRate)) {
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

void resampleBufferToMono(const AudioBuffer<float>& inBuffer,
                          AudioBuffer<float>& outBuffer,
                          double inSourceSampleRate,
                          double inTargetSampleRate)
{
    const int num_channels = inBuffer.getNumChannels();

    if (num_channels < 1) {
        // Nothing to average. Caught here rather than left to divide by zero downstream.
        jassertfalse;
        outBuffer.setSize(1, 0);
        return;
    }

    if (juce::exactlyEqual(inSourceSampleRate, inTargetSampleRate)) {
        outBuffer.setSize(1, inBuffer.getNumSamples());
        outBuffer.copyFrom(0, 0, inBuffer, 0, 0, inBuffer.getNumSamples());

        for (int ch = 1; ch < num_channels; ch++) {
            outBuffer.addFrom(0, 0, inBuffer, ch, 0, inBuffer.getNumSamples());
        }

        outBuffer.applyGain(1.0f / static_cast<float>(num_channels));
        return;
    }

    Resampler resampler;
    resampler.prepareToPlay(inSourceSampleRate, inBuffer.getNumSamples(), inTargetSampleRate);
    const auto num_expected_samples_after_resample =
        resampler.getNumOutSamplesOnNextProcessBlock(inBuffer.getNumSamples());

    outBuffer.setSize(1, num_expected_samples_after_resample);

    // One pass over the whole thing, channels folded in as they are read.
    const int num_samples_after_resample = resampler.processBlock(
        inBuffer.getArrayOfReadPointers(), num_channels, outBuffer.getWritePointer(0), inBuffer.getNumSamples());
    jassertquiet(num_samples_after_resample == num_expected_samples_after_resample);
}

bool _loadMP3File(const std::string& filename, juce::AudioBuffer<float>& outBuffer, double& outSampleRate)
{
    mp3dec_t mp3d;
    mp3dec_file_info_t info;
    int loadResult = mp3dec_load(&mp3d, filename.c_str(), &info, nullptr, nullptr);

    if (loadResult) {
        return false;
    }

    const auto num_channels = static_cast<size_t>(info.channels);

    outBuffer.setSize(info.channels, static_cast<int>(info.samples / num_channels));

    for (size_t i = 0; i < info.samples; ++i) {
        size_t channel = i % num_channels;
        outBuffer.setSample((int) channel, static_cast<int>(i / num_channels), (float) info.buffer[i] / 32768.0f);
    }

    outSampleRate = static_cast<double>(info.hz);
    free(info.buffer);
    return true;
}

} // namespace AudioUtils