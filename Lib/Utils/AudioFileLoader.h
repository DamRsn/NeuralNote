//
// Created by Damien Ronssin on 09.03.23.
//

#ifndef AudioFileLoader_h
#define AudioFileLoader_h

#include <JuceHeader.h>

#include "DownSampler.h"
#include "Constants.h"

class AudioFileLoader
{
public:
    AudioFileLoader();

    ~AudioFileLoader();

    bool loadAudioFile(const juce::File& inFile,
                       AudioBuffer<float>& outBuffer,
                       int& outNumOutSamples);

private:
    DownSampler mDownSampler;

    AudioFormatManager mAudioFormatManager;
};

#endif // AudioFileLoader_h
