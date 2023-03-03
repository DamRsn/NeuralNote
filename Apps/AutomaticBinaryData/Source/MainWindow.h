#pragma once

#include "MainComponent.h"

class MainWindow : public DocumentWindow
{
public:
    explicit MainWindow(const String& name);

private:
    void closeButtonPressed() override;
    Colour getBackgroundColour();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};
