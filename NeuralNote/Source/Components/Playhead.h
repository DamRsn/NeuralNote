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

    /** The marker at the top of the line. Only the waveform shows one; the ruler and the piano
        roll continue the same playhead and would otherwise repeat it twice more. */
    void setDrawTriangle(bool inShouldDraw);

    /** @return The playhead's x in this component, for a view that shades what has been played. */
    double getPlayheadX() const;

private:
    void _onVBlankCallback();

    NeuralNoteAudioProcessor* mProcessor;
    VBlankAttachment mVBlankAttachment;

    double mCurrentPlayerPlayheadTime = 0;
    double mAudioSampleDuration = 0;
    double mZoomLevel = 1.0;
    const double mBaseNumPixelsPerSecond;

    bool mDrawTriangle = true;

    // Whether the last frame drew anything, so the frame that starts or stops drawing repaints in
    // full rather than only the sliver the line moved through.
    bool mIsDrawing = false;

    static constexpr float mTriangleSide = 9.0f;
    static constexpr float mTriangleHeight = 0.86602540378f * mTriangleSide; // Sqrt(3) / 2
};

#endif // Playhead_h
