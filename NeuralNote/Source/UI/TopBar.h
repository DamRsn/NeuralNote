//
// Created by Damien Ronssin on 07.08.26.
//

#ifndef TopBar_h
#define TopBar_h

#include <optional>

#include <JuceHeader.h>

#include "NnFlatButton.h"
#include "NnFlatSlider.h"
#include "TimeDisplay.h"

class NeuralNoteAudioProcessor;

/**
 * The window's top strip: wordmark, transport, position readout, model, mix, output level, the
 * passthrough mute and the settings button.
 */
class TopBar : public juce::Component
{
public:
    explicit TopBar(NeuralNoteAudioProcessor& inProcessor);

    void paint(juce::Graphics& g) override;

    void resized() override;

    /** Re-derives which transport buttons are live from the processor's state. */
    void updateEnablements();

    /** Pulls the play and record toggles back into step with what the engine is actually doing. */
    void syncTransportToggles();

    NnFlatButton& getBackButton() { return mBackButton; }

    NnFlatButton& getPlayPauseButton() { return mPlayPauseButton; }

    NnFlatButton& getRecordButton() { return mRecordButton; }

    NnFlatButton& getFollowButton() { return mFollowButton; }

    NnFlatButton& getMuteButton() { return mMuteButton; }

    NnFlatButton& getSettingsButton() { return mSettingsButton; }

    NnFlatButton& getModelButton() { return mModelButton; }

    /**
     * Names the model a transcription would use, and lights the button while the model panel is up.
     * Cheap to call often: it acts only on a change.
     */
    void syncModelButton(bool inModelPanelVisible);

private:
    void _paintMixPill(juce::Graphics& g, float inAlpha) const;

    void _paintVolumePill(juce::Graphics& g, float inAlpha) const;

    /**
     * Dims the mix pill while there is no synth side to balance against -- the Player is what
     * actually holds the mix to the source. Cheap to call often: it acts only on a change.
     */
    void _refreshMixAvailability();

    bool _hasTranscription() const;

    NeuralNoteAudioProcessor& mProcessor;

    NnFlatButton mBackButton {"Back"};
    NnFlatButton mPlayPauseButton {"PlayPause"};
    NnFlatButton mLoopButton {"Loop"};
    NnFlatButton mFollowButton {"Follow"};
    NnFlatButton mRecordButton {"Record"};

    // Which of the two icons mPlayPauseButton is carrying, so the 20 Hz sync can skip setting
    // an icon that is already there. The constructor sets play.
    bool mShowingPauseIcon = false;

    TimeDisplay mTimeDisplay;

    NnFlatButton mModelButton {"Model"};
    juce::String mModelButtonLabel;

    NnFlatSlider mMixSlider;
    NnFlatSlider mMasterGainSlider;

    NnFlatButton mMuteButton {"Mute"};
    NnFlatButton mSettingsButton {"Settings"};

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> mMuteAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mMixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mMasterGainAttachment;

    juce::Rectangle<int> mMixPill;
    juce::Rectangle<int> mVolumePill;

    // Unset until first evaluated, so the opening refresh always applies.
    std::optional<bool> mMixHeldToOriginal;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TopBar)
};

#endif // TopBar_h
