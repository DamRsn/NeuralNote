#pragma once

#include "HelloWorldLabel.h"

namespace GuiApp
{

class MainComponent : public Component
{
public:
    MainComponent();

    void paint(Graphics&) override;
    void resized() override;

private:
    HelloWorldLabel helloWorld;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

} // namespace GuiApp
