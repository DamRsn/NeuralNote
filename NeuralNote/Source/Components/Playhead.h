//
// Created by Damien Ronssin on 17.06.23.
//

#ifndef Playhead_h
#define Playhead_h

#include "PluginProcessor.h"
#include <JuceHeader.h>

class Playhead
    : public Component
    , public Timer
{
public:
    Playhead(NeuralNoteAudioProcessor* inProcessor);

    void resized() override;

    void paint(juce::Graphics& g) override;

    void timerCallback() override;

    void setPlayheadTime(double inNewTime);

private:
    NeuralNoteAudioProcessor* mProcessor;

    double mCurrentPlayerPlayheadTime = 0;
    double mAudioSampleDuration = 0;

    static constexpr float mTriangleSide = 8.0f;
    static constexpr float mTriangleHeight = 0.86602540378 * mTriangleSide; // Sqrt(3) / 2
};

#endif // Playhead_h
