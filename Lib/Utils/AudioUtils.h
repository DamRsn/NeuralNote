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
     * Load an audio file
     * @param inFile
     * @param outBuffer
     * @param outSampleRate
     * @return
     */
bool loadAudioFile(const juce::File& inFile, AudioBuffer<float>& outBuffer, double& outSampleRate);

void resampleBuffer(const AudioBuffer<float>& inBuffer,
                    AudioBuffer<float>& outBuffer,
                    double inSourceSampleRate,
                    double inTargetSampleRate);

}; // namespace AudioUtils

#endif // AudioFileLoader_h
