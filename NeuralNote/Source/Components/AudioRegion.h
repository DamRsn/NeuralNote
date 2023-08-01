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
class VisualizationPanel;

class AudioRegion : public Component
{
public:
    AudioRegion(NeuralNoteAudioProcessor& processor,
                double inNumPixelsPerSecond,
                VisualizationPanel* inVisualizationPanel);

    void resized() override;

    void paint(Graphics& g) override;

    void setIsFileOver(bool inIsFileOver);

    void setThumbnailWidth(int inThumbnailWidth);

    void mouseDown(const juce::MouseEvent& e) override;

    void mouseEnter(const juce::MouseEvent& event) override;

    void mouseExit(const juce::MouseEvent& event) override;

private:
    NeuralNoteAudioProcessor& mProcessor;

    VisualizationPanel* mVisualizationPanel;

    float _timeToPixel(float inTime) const;

    float _pixelToTime(float inPixel) const;

    Playhead mPlayhead;

    std::shared_ptr<juce::FileChooser> mFileChooser;

    const double mNumPixelsPerSecond;

    int mThumbnailWidth = 0;
    bool mIsFileOver = false;
};

#endif // AudioRegion_h
