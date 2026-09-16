//
// Created by Damien Ronssin on 14.08.26.
//

#ifndef NnLevelMeter_h
#define NnLevelMeter_h

#include <JuceHeader.h>

/**
 * A segmented level meter: a row of rectangles, each drawn lit or unlit and never part way.
 *
 * A component of its own rather than something the strip paints, so a level change repaints three
 * pixels instead of the whole 76 px row -- and it only repaints when the number of lit segments
 * actually changes, which at 120 Hz over a dozen strips is the entire cost of the feature.
 *
 * The segment colours are shared by every meter and come from nn::colours; only the unlit colour
 * differs between the strips and the master, so that is the one ColourId.
 */
class NnLevelMeter : public juce::Component
{
public:
    enum ColourIds {
        unlitColourId = 0x6e9b210,
    };

    NnLevelMeter(int inNumSegments, int inGap);

    /**
     * @param inInstantDb This frame's level, already converted from the audio thread's mean square.
     * @param inDtSeconds Since the previous frame, for the release. Zero on the first frame.
     */
    void setLevelDb(float inInstantDb, float inDtSeconds);

    void paint(juce::Graphics& g) override;

private:
    const int mNumSegments;
    const int mGap;

    float mDisplayDb;
    int mLitSegments = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NnLevelMeter)
};

#endif // NnLevelMeter_h
