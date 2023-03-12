//
// Created by Damien Ronssin on 12.03.23.
//

#ifndef TranscriptionOptionsView_h
#define TranscriptionOptionsView_h

#include <JuceHeader.h>

#include "PluginProcessor.h"
#include "Knob.h"

class TranscriptionOptionsView : public Component
{
public:
    TranscriptionOptionsView(Audio2MidiAudioProcessor& processor);

    void resized() override;

    void paint(Graphics& g) override;

private:
    Audio2MidiAudioProcessor& mProcessor;

    std::unique_ptr<Knob> mNoteSensibility;
    std::unique_ptr<Knob> mSplitSensibility;
    std::unique_ptr<Knob> mMinNoteDuration;

    std::unique_ptr<juce::ComboBox> mPitchBendDropDown;
};

#endif // TranscriptionOptionsView_h
