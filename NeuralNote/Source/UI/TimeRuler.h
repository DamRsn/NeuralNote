//
// Created by Damien Ronssin on 07.08.26.
//

#ifndef TimeRuler_h
#define TimeRuler_h

#include <JuceHeader.h>

#include "Playhead.h"

class NeuralNoteAudioProcessor;

/**
 * The time axis between the waveform and the piano roll.
 *
 * Absolute seconds, with no bar or beat grid anywhere: the source recording has no tempo, and the
 * only tempo in the UI is the one written into an exported MIDI file. Lives inside the same
 * scrolling region as the waveform and the piano roll, so the three axes cannot drift apart.
 */
class TimeRuler : public juce::Component
{
public:
    TimeRuler(NeuralNoteAudioProcessor* inProcessor, double inBaseNumPixelsPerSecond);

    void paint(juce::Graphics& g) override;

    void resized() override;

    void setZoomLevel(double inZoomLevel);

private:
    /** @return The tick spacing in seconds that keeps labels at least MIN_LABEL_GAP apart. */
    double _chooseDivision() const;

    NeuralNoteAudioProcessor* mProcessor;

    Playhead mPlayhead;

    double mBaseNumPixelsPerSecond;
    double mZoomLevel = 1.0;

    static constexpr int MIN_LABEL_GAP = 56;

    // Well under CombinedAudioMidiRegion's own minimum; only here to keep paint()'s tick walk
    // advancing.
    static constexpr double MIN_ZOOM_LEVEL = 1e-3;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimeRuler)
};

#endif // TimeRuler_h
