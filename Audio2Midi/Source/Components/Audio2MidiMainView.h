//
// Created by Damien Ronssin on 06.03.23.
//

#ifndef PluginMainView_h
#define PluginMainView_h

#include <JuceHeader.h>
#include "PluginProcessor.h"

class Audio2MidiMainView : public juce::Component
{
public:
    Audio2MidiMainView(Audio2MidiAudioProcessor& processor);

    ~Audio2MidiMainView();

    void resized() override;

    void paint(juce::Graphics& g) override;

private:
    Audio2MidiAudioProcessor& mProcessor;

    std::unique_ptr<juce::Slider> mGainSlider;

    std::unique_ptr<juce::TextButton> mRecordButton;

    std::unique_ptr<juce::TextButton> mClearButton;
};

#endif // PluginMainView_h
