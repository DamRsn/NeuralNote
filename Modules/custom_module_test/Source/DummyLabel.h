#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace CustomModule
{
class DummyLabel : public juce::Component
{
public:
    DummyLabel();

    void resized() override;

private:
    juce::Label label;
};

} // namespace CustomModule
