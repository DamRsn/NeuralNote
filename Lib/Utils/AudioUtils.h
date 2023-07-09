//
// Created by Damien Ronssin on 09.03.23.
//

#ifndef AudioFileLoader_h
#define AudioFileLoader_h

#include <JuceHeader.h>

#include "Resampler.h"
#include "BasicPitchConstants.h"

namespace AudioUtils
{

/**
     * Load an audio file. *.wav, *.aiff, *.flac formats supported
     * @param inFile Audio file to load.
     * @param outBuffer Buffer where to load audio data. Will be resized to correct number of channels and samples.
     * @param outSampleRate Will be set to audio file sample rate.
     * @return Whether the load was sucessfull or not.
     */
bool loadAudioFile(const juce::File& inFile, AudioBuffer<float>& outBuffer, double& outSampleRate);

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

}; // namespace AudioUtils

#endif // AudioFileLoader_h
