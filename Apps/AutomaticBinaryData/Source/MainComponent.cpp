
#include "MainComponent.h"
#include "BinaryHelper.h"

MainComponent::MainComponent()
{
    for (auto& image: getBinaryDataImages())
    {
        images.emplace_back(std::make_unique<ImageComponent>());
        images.back()->setImage(image);
        addAndMakeVisible(*images.back());
    }

    setSize (600, 400);
}

void MainComponent::paint (Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    float y = 0.f;
    float height = 1.f / (float)images.size();

    for (auto& image: images)
    {
        image->setBoundsRelative(0.f, y, 1.f, height);
        y+=height;
    }
}
