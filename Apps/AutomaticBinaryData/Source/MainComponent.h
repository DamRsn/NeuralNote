#pragma once

#include "CommonHeaders.h"

class MainComponent   : public Component
{
public:

    MainComponent();

    void paint (Graphics&) override;
    void resized() override;

private:
    std::vector<std::unique_ptr<ImageComponent>> images;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
