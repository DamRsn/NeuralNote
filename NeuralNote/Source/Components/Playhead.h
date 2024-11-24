//
// Created by Damien Ronssin on 17.06.23.
//

#ifndef Playhead_h
#define Playhead_h

#include "PluginProcessor.h"
#include <JuceHeader.h>

class Playhead : public Component
{
public:
    Playhead(NeuralNoteAudioProcessor* inProcessor, double inNumPixelsPerSecond);

    void resized() override;

    void paint(juce::Graphics& g) override;

    void setPlayheadTime(double inNewTime);

    static double computePlayheadPositionPixel(double inPlayheadPositionSeconds,
                                               double inSampleDuration,
                                               double inBaseNumPixelPerSecond,
                                               double inZoomLevel,
                                               int inWidth);

    void setZoomLevel(double inZoomLevel);

private:
    void _onVBlankCallback();

    NeuralNoteAudioProcessor* mProcessor;
    VBlankAttachment mVBlankAttachment;

    double mCurrentPlayerPlayheadTime = 0;
    double mAudioSampleDuration = 0;
    double mZoomLevel = 1.0;
    const double mBaseNumPixelsPerSecond;

    static constexpr float mTriangleSide = 8.0f;
    static constexpr float mTriangleHeight = 0.86602540378 * mTriangleSide; // Sqrt(3) / 2
};

#endif // Playhead_h
