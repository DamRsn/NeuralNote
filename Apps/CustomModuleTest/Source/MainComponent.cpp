#include "MainComponent.h"

namespace GuiApp
{
MainComponent::MainComponent()
{
    addAndMakeVisible(dummyLabel);
    setSize(600, 400);
}

void MainComponent::paint(Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    dummyLabel.setBounds(getLocalBounds());
}
} // namespace GuiApp