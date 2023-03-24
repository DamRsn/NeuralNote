#include "PluginProcessor.h"
#include "PluginEditor.h"

NeuralNoteEditor::NeuralNoteEditor(NeuralNoteAudioProcessor& p)
    : AudioProcessorEditor(&p)
{
    mMainView = std::make_unique<NeuralNoteMainView>(p);

    addAndMakeVisible(*mMainView);
    setSize(1000, 640);

    getLookAndFeel().setDefaultSansSerifTypeface(MONTSERRAT_REGULAR);

    mMainView->setLookAndFeel(&mNeuralNoteLnF);
}

NeuralNoteEditor::~NeuralNoteEditor()
{
    mMainView->setLookAndFeel(nullptr);
}

void NeuralNoteEditor::paint(juce::Graphics& g)
{
}

void NeuralNoteEditor::resized()
{
    mMainView->setBounds(getLocalBounds());
}
