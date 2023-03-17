//
// Created by Damien Ronssin on 17.03.23.
//

#ifndef NeuralNoteLNF_h
#define NeuralNoteLNF_h

#include "JuceHeader.h"
#include "UIDefines.h"

class NeuralNoteLNF : public juce::LookAndFeel_V4
{
public:
    NeuralNoteLNF();

    Font getComboBoxFont(ComboBox& /*box*/) override { return LABEL_FONT; }

    Font getPopupMenuFont() override { return LABEL_FONT; }

    Font getTextButtonFont(TextButton&, int buttonHeight) override { return LARGE_FONT; };

    void drawRotarySlider(Graphics&,
                          int x,
                          int y,
                          int width,
                          int height,
                          float sliderPosProportional,
                          float rotaryStartAngle,
                          float rotaryEndAngle,
                          Slider&) override;
};

#endif // NeuralNoteLNF_h
