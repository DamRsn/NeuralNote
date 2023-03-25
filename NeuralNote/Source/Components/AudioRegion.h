//
// Created by Damien Ronssin on 09.03.23.
//

#ifndef AudioRegion_h
#define AudioRegion_h

#include <JuceHeader.h>

#include "AudioFileLoader.h"
#include "PluginProcessor.h"
#include "UIDefines.h"

class AudioRegion : public Component
{
public:
    AudioRegion(NeuralNoteAudioProcessor& processor);

    void resized() override;

    void paint(Graphics& g) override;

    void updateThumbnail();

    void setIsFileOver(bool inIsFileOver);

    bool onFileDrop(const juce::File& inFile);

    void setThumbnailWidth(int inThumbnailWidth);

    void mouseDown(const juce::MouseEvent& e) override;

private:
    NeuralNoteAudioProcessor& mProcessor;

    int mThumbnailWidth = 0;

    const int mSourceSamplesPerThumbnailSample = 128;
    juce::AudioFormatManager mThumbnailFormatManager;
    juce::AudioThumbnailCache mThumbnailCache;
    juce::AudioThumbnail mThumbnail;
    
    AudioFileLoader mFileLoader;

    bool mIsFileOver = false;
};

#endif // AudioRegion_h
