//
// Created by Damien Ronssin on 07.08.26.
//

#ifndef TimelineGutter_h
#define TimelineGutter_h

#include <JuceHeader.h>

/**
 * The fixed 46 px column to the left of the waveform and the ruler.
 *
 * It exists so those two reserve exactly the width the piano roll's keyboard takes, which is what
 * keeps all three time axes aligned. The waveform's share carries the amplitude scale; the ruler's
 * is deliberately empty.
 */
class TimelineGutter : public juce::Component
{
public:
    /** @param inWaveformHeight Height of the waveform's share, above the ruler's. */
    explicit TimelineGutter(int inWaveformHeight);

    void paint(juce::Graphics& g) override;

private:
    int mWaveformHeight;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimelineGutter)
};

#endif // TimelineGutter_h
