//
// Created by Damien Ronssin on 12.03.23.
//

#ifndef RhythmOptionsView_h
#define RhythmOptionsView_h

#include <JuceHeader.h>

#include "PluginProcessor.h"
#include "UIDefines.h"
#include "RhythmUtils.h"

class RhythmOptionsView : public Component
{
public:
    RhythmOptionsView(Audio2MidiAudioProcessor& processor);

    void resized() override;

    void paint(Graphics& g) override;

private:
    void _valueChanged();

    const int mTopPad = 23;
    Audio2MidiAudioProcessor& mProcessor;

    std::unique_ptr<juce::ComboBox> mTimeDivisionDropdown;

    std::unique_ptr<juce::Slider> mQuantization;

    std::unique_ptr<juce::TextButton> mSnapMode;
};

#endif // RhythmOptionsView_h
