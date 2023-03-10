//
// Created by Damien Ronssin on 06.03.23.
//

#ifndef PluginMainView_h
#define PluginMainView_h

#include <JuceHeader.h>

#include "AudioRegion.h"
#include "PluginProcessor.h"
#include "RotarySlider.h"

class Audio2MidiMainView
    : public juce::Component
    , public juce::Slider::Listener
    , public juce::Timer
{
public:
    Audio2MidiMainView(Audio2MidiAudioProcessor& processor);

    ~Audio2MidiMainView();

    void resized() override;

    void paint(juce::Graphics& g) override;

    void timerCallback() override;

private:
    void sliderValueChanged(juce::Slider* inSliderPtr) override;

    void updateEnablement();

    Audio2MidiAudioProcessor& mProcessor;

    AudioRegion mAudioRegion;

    std::unique_ptr<juce::Slider> mGainSlider;
    std::unique_ptr<juce::Button> mMuteButton;

    std::unique_ptr<juce::TextButton> mRecordButton;
    std::unique_ptr<juce::TextButton> mClearButton;

    std::unique_ptr<RotarySlider> mNoteSegmentationSlider;
    std::unique_ptr<RotarySlider> mModelConfidenceThresholdSlider;
    std::unique_ptr<RotarySlider> mMinNoteDuration;

    std::unique_ptr<RotarySlider> mMinNoteSlider;
    std::unique_ptr<RotarySlider> mMaxNoteSlider;
    std::unique_ptr<juce::ToggleButton> mPitchBendCheckbox;

    std::unique_ptr<ComboBox> mKey; // C, C#, D, D# ...
    std::unique_ptr<ComboBox> mMode; // Major, Minor

    // Eventually quantise functionality: need for time division, quantize force (0 - 100)
};

#endif // PluginMainView_h
