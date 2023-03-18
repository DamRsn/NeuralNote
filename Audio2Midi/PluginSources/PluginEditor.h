#pragma once

#include "PluginProcessor.h"
#include "Audio2MidiMainView.h"
#include "NeuralNoteLNF.h"

class Audio2MidiEditor : public juce::AudioProcessorEditor
{
public:
    explicit Audio2MidiEditor(Audio2MidiAudioProcessor&);

    ~Audio2MidiEditor();

    void paint(juce::Graphics&) override;

    void resized() override;

private:
    std::unique_ptr<Audio2MidiMainView> mMainView;

    NeuralNoteLNF mNeuralNoteLnF;
};
