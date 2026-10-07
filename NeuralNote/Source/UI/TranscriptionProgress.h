//
// Created by Damien Ronssin on 12.08.26.
//

#ifndef TranscriptionProgress_h
#define TranscriptionProgress_h

#include <JuceHeader.h>

#include "MuscriptorEngine.h"
#include "NnFlatButton.h"

class NeuralNoteAudioProcessor;

/**
 * What an unfinished transcription looks like: a caption, a bar, a percentage, and a button that
 * pauses it while it runs and resumes it once paused.
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
    /** Polls the engine's progress; only repaints when the phase, the percentage or the pulse moved. */
    void _onVBlankCallback();

    /** Shows pause while running and resume while paused. */
    void _updateButton(bool inIsPaused);

    NeuralNoteAudioProcessor& mProcessor;

    NnFlatButton mPauseResumeButton {"PauseResumeTranscription"};

    juce::VBlankAttachment mVBlankAttachment;

    // Mirrors the engine's pending stop request, from either this button or the toolbar's bin, so a
    // second click does not read as the first one having done nothing. Stopping cannot interrupt GPU
    // initialisation, which can take seconds.
    bool mIsCancelling = false;

    bool mIsPaused = false;

    MuscriptorEngine::Phase mDisplayedPhase = MuscriptorEngine::Phase::LoadingModel;
    // Negative while the phase has no measurable progress yet.
    int mDisplayedPercent = -1;
    float mPulse = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TranscriptionProgress)
};

#endif // TranscriptionProgress_h
