//
// Created by Damien Ronssin on 07.08.26.
//

#ifndef TimeDisplay_h
#define TimeDisplay_h

#include <JuceHeader.h>

class NeuralNoteAudioProcessor;

/**
 * The transport's position and total duration, as `mm:ss.dd / mm:ss.dd`.
 *
 * Driven by a vblank callback rather than a timer, and repainted only when the formatted strings
 * actually change -- at hundredths that is roughly every other frame, and it keeps a stopped
 * transport from repainting at all.
 */
class TimeDisplay : public juce::Component
{
public:
    explicit TimeDisplay(NeuralNoteAudioProcessor* inProcessor);

    void paint(juce::Graphics& g) override;

    /** @return The width the display needs for its text and padding. Fixed: the font is monospaced
        and the format has no variable-width part, so it does not depend on the current time. */
    static int getIdealWidth();

private:
    void _onVBlankCallback();

    static juce::String _format(double inSeconds);

    NeuralNoteAudioProcessor* mProcessor;

    juce::VBlankAttachment mVBlankAttachment;

    juce::String mPosition {"00:00.00"};
    juce::String mTotal {"00:00.00"};
};

#endif // TimeDisplay_h
