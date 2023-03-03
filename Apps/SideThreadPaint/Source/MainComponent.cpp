#include "MainComponent.h"

namespace GuiApp
{
MainComponent::MainComponent()
{
    for (int index = 0; index < 100; ++index)
    {
        paths.emplace_back(std::make_unique<ComplicatedPath>());
        addAndMakeVisible(paths.back().get());
    }

    setSize(600, 400);
}

void MainComponent::paint(Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    float y = 0.f;
    float height = 1.f / (float) paths.size();

    for (auto& path: paths)
    {
        path->setBoundsRelative(0.f, y, 1.f, height);
        y += height;
    }
}

} // namespace GuiApp
