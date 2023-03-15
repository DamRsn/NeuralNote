//
// Created by Damien Ronssin on 09.03.23.
//

#ifndef AudioRegion_h
#define AudioRegion_h

#include <JuceHeader.h>

#include "AudioFileLoader.h"
#include "PluginProcessor.h"

class AudioRegion : public Component
{
public:
    AudioRegion(Audio2MidiAudioProcessor& processor);

    void resized() override;

    void paint(Graphics& g) override;

    void updateThumbnail();

    void setIsFileOver(bool inIsFileOver);

    bool onFileDrop(const juce::File& inFile);

    void setThumbnailWidth(int inThumbnailWidth);

private:
    Audio2MidiAudioProcessor& mProcessor;

    int mThumbnailWidth = 0;

    const int mSourceSamplesPerThumbnailSample = 256;
    juce::AudioFormatManager mThumbnailFormatManager;
    juce::AudioThumbnailCache mThumbnailCache;
    juce::AudioThumbnail mThumbnail;

    AudioFileLoader mFileLoader;

    bool mIsFileOver = false;
};

#endif // AudioRegion_h
