//
// Created by Damien Ronssin on 09.03.23.
//

#ifndef AudioUtils_h
#define AudioUtils_h

#include <iostream>
#include <fstream>
#include <iterator>
#include <string>

#include <JuceHeader.h>

#include "Resampler.h"
#include "TranscriptionConstants.h"

namespace AudioUtils
{

/**
 * Load an audio file. wav, aiff, flac, ogg vorbis and mp3 files are supported.
 * @param inFile Audio file to load.
 * @param outBuffer Buffer where to load audio data. Will be resized to correct number of channels and samples.
 * @param outSampleRate Will be set to audio file sample rate.
 * @return Whether the load was sucessfull or not.
 */
bool loadAudioFile(const juce::File& inFile, AudioBuffer<float>& outBuffer, double& outSampleRate);

/**
 * @brief Get the Supported Audio File Extensions object (.wav, .aiff, .flac, .ogg, .mp3 ...)
 * 
 * @return StringArray containing all suported file extensions
 */
StringArray getSupportedAudioFileExtensions();

/**
 * Create an AudioFormatManager with all supported audio formats registered. 
 * @return std::unique_ptr<AudioFormatManager>
 */
std::unique_ptr<AudioFormatManager> createAudioFormatManager();

/**
 * Resample an audio buffer from source sample rate to target sample rate.
 * Filters are applied if needed to prevent any aliasing.
 * @param inBuffer Audio buffer to resample
 * @param outBuffer Audio buffer where to store resampled audio. Will be resized to correct size and same number of channels as inBuffer.
 * @param inSourceSampleRate Sample rate of inBuffer.
 * @param inTargetSampleRate Target sample rate.
 */
void resampleBuffer(const AudioBuffer<float>& inBuffer,
                    AudioBuffer<float>& outBuffer,
                    double inSourceSampleRate,
                    double inTargetSampleRate);

/**
 * Resample an audio buffer to a single channel, averaging the input channels as part of the same
 * pass. Filters are applied if needed to prevent any aliasing.
 *
 * This is what the transcription model is fed. It has to be one channel, and it has to be the whole
 * mix: anything panned away from centre is attenuated or absent in a single source channel, and the
 * model transcribes what it hears.
 *
 * @param inBuffer Audio buffer to resample, any number of channels.
 * @param outBuffer Where to store the result. Resized to one channel.
 * @param inSourceSampleRate Sample rate of inBuffer.
 * @param inTargetSampleRate Target sample rate.
 */
void resampleBufferToMono(const AudioBuffer<float>& inBuffer,
                          AudioBuffer<float>& outBuffer,
                          double inSourceSampleRate,
                          double inTargetSampleRate);

/**
 * Load an mp3 file
 * @param filename path to mp3 file to read
 * @param outBuffer output buffer on which to read the data
 * @param outSampleRate File sample rate
 * @return Whether file load was a success
 */
bool _loadMP3File(const std::string& filename, juce::AudioBuffer<float>& outBuffer, double& outSampleRate);

} // namespace AudioUtils

#endif // AudioUtils_h
