#pragma once

#include "CommonHeader.h"

namespace AudioApp
{
class MainComponent : public juce::AudioAppComponent
{
public:
    MainComponent();

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;

    void paint(Graphics&) override;
    void resized() override;

private:
    juce::AudioDeviceSelectorComponent selector {
        deviceManager, 2, 2, 2, 2, false, false, true, false};
    WhiteNoise::Oscillator noise;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

} // namespace AudioApp
