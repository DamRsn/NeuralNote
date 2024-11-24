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
    AudioRegion(NeuralNoteAudioProcessor* processor, double inBaseNumPixelsPerSecond);

    void resized() override;

    void paint(Graphics& g) override;

    void setIsFileOver(bool inIsFileOver);

    void setThumbnailWidth(int inThumbnailWidth);

    void mouseDown(const juce::MouseEvent& e) override;

    void setZoomLevel(double inZoomLevel);

private:
    NeuralNoteAudioProcessor* mProcessor;

    float _pixelToTime(float inPixel) const;

    Playhead mPlayhead;

    std::shared_ptr<juce::FileChooser> mFileChooser;

    const double mBaseNumPixelsPerSecond;
    double mZoomLevel = 1.0;

    int mThumbnailWidth = 0;
    bool mIsFileOver = false;
};

#endif // AudioRegion_h
