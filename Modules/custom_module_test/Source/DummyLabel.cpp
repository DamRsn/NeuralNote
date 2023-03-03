#include "DummyLabel.h"

namespace CustomModule
{
DummyLabel::DummyLabel()
{
    addAndMakeVisible(label);

    label.setFont(juce::Font(22));
    label.setText("This code runs from a custom module", juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
}

void DummyLabel::resized()
{
    label.setBounds(getLocalBounds());
}

} // namespace CustomModule
