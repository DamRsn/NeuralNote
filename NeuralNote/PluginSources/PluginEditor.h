#pragma once

#include "PluginProcessor.h"
#include "NeuralNoteMainView.h"
#include "NeuralNoteLNF.h"

class NeuralNoteEditor : public juce::AudioProcessorEditor
{
public:
    explicit NeuralNoteEditor(NeuralNoteAudioProcessor&);

    ~NeuralNoteEditor();

    void paint(juce::Graphics&) override;

    void resized() override;

    NeuralNoteMainView* getMainView() const { return mMainView.get(); }

private:
    std::unique_ptr<NeuralNoteMainView> mMainView;

    NeuralNoteLNF mNeuralNoteLnF;
};
