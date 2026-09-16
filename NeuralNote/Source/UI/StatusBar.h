//
// Created by Damien Ronssin on 07.08.26.
//

#ifndef StatusBar_h
#define StatusBar_h

#include <JuceHeader.h>

#include "NnFlatSlider.h"
#include "TranscriptionProgress.h"

class NeuralNoteAudioProcessor;

/**
 * The strip along the bottom: what was transcribed on the left, the piano roll's vertical zoom on
 * the right, and -- while a transcription runs -- its progress centred over the roll.
 *
 * The figures repaint on the mixer's change messages and on the main view's state changes, not on a
 * timer; nothing there moves while the transport runs. The progress group drives its own repaints.
 */
class StatusBar
    : public juce::Component
    , private juce::ChangeListener
{
public:
    explicit StatusBar(NeuralNoteAudioProcessor& inProcessor);

    ~StatusBar() override;

    void resized() override;

    void paint(juce::Graphics& g) override;

    /** Re-derives whether the progress group is showing. */
    void updateEnablements();

    /** Called with the slider's normalised position whenever the user moves it. */
    std::function<void(float)> onVerticalZoomChange;

    /** Moves the slider without calling back, for the zoom the view picked for itself. */
    void setVerticalZoom(float inNorm);

private:
    void changeListenerCallback(juce::ChangeBroadcaster* inSource) override;

    NeuralNoteAudioProcessor& mProcessor;

    TranscriptionProgress mProgress;
    NnFlatSlider mZoomSlider;

    juce::Rectangle<int> mZoomIconBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StatusBar)
};

#endif // StatusBar_h
