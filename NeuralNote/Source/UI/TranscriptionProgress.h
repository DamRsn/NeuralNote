//
// Created by Damien Ronssin on 12.08.26.
//

#ifndef TranscriptionProgress_h
#define TranscriptionProgress_h

#include <JuceHeader.h>

#include "NnFlatButton.h"

class NeuralNoteAudioProcessor;

/**
 * What a running transcription looks like: a caption, a bar, a percentage and a way to stop.
 *
 * Lives in the status bar and is never modal -- the roll fills in as the run goes, so nothing may
 * cover it.
 */
class TranscriptionProgress : public juce::Component
{
public:
    explicit TranscriptionProgress(NeuralNoteAudioProcessor& inProcessor);

    /** @return The width the whole group needs, for the caller to centre it. */
    static int getIdealWidth();

    void resized() override;

    void paint(juce::Graphics& g) override;

private:
    /** Polls the engine's progress; only repaints when the percentage or the pulse moved. */
    void _onVBlankCallback();

    NeuralNoteAudioProcessor& mProcessor;

    NnFlatButton mCancelButton {"CancelTranscription"};

    juce::VBlankAttachment mVBlankAttachment;

    // Latched so a second click does not read as the first one having done nothing. Cancellation is
    // observed at a chunk boundary, which can be seconds away.
    bool mIsCancelling = false;

    int mDisplayedPercent = 0;
    float mPulse = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TranscriptionProgress)
};

#endif // TranscriptionProgress_h
