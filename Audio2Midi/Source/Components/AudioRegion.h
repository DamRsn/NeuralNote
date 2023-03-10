//
// Created by Damien Ronssin on 09.03.23.
//

#ifndef AudioRegion_h
#define AudioRegion_h

#include <JuceHeader.h>

#include "AudioFileLoader.h"
#include "PluginProcessor.h"

class AudioRegion
    : public Component
    , public FileDragAndDropTarget
{
public:
    AudioRegion(Audio2MidiAudioProcessor& processor);

    void resized() override;

    void paint(Graphics& g) override;

    bool isInterestedInFileDrag(const StringArray& files) override;

    void filesDropped(const StringArray& files, int x, int y) override;

    void fileDragEnter(const StringArray& files, int x, int y) override;

    void fileDragExit(const StringArray& files) override;

private:
    Audio2MidiAudioProcessor& mAudioProcessor;

    const int mSourceSamplesPerThumbnailSample = 512;
    juce::AudioFormatManager mThumbnailFormatManager;
    juce::AudioThumbnailCache mThumbnailCache;
    juce::AudioThumbnail mThumbnail;

    AudioFileLoader mFileLoader;

    bool mIsFileOver = false;
};

#endif // AudioRegion_h
