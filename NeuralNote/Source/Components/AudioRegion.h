//
// Created by Damien Ronssin on 09.03.23.
//

#ifndef AudioRegion_h
#define AudioRegion_h

#include <JuceHeader.h>

#include "AudioUtils.h"
#include "PluginProcessor.h"
#include "UIDefines.h"
#include "Playhead.h"

class CombinedAudioMidiRegion;

class AudioRegion : public Component
{
public:
    AudioRegion(NeuralNoteAudioProcessor* processor, double inNumPixelsPerSecond);

    void resized() override;

    void paint(Graphics& g) override;

    void setIsFileOver(bool inIsFileOver);

    void setThumbnailWidth(int inThumbnailWidth);

    void mouseDown(const juce::MouseEvent& e) override;

private:
    NeuralNoteAudioProcessor* mProcessor;

    float _pixelToTime(float inPixel) const;

    Playhead mPlayhead;

    std::shared_ptr<juce::FileChooser> mFileChooser;

    const double mNumPixelsPerSecond;

    int mThumbnailWidth = 0;
    bool mIsFileOver = false;
};

#endif // AudioRegion_h
