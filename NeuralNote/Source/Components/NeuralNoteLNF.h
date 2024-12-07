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

    Font getComboBoxFont(ComboBox& /*box*/) override { return UIDefines::LABEL_FONT(); }

    Font getPopupMenuFont() override { return UIDefines::LABEL_FONT(); }

    Font getTextButtonFont(TextButton&, int buttonHeight) override { return UIDefines::LARGE_FONT(); };

    Font getLabelFont(juce::Label&) override { return UIDefines::DROPDOWN_FONT(); };

    void drawRotarySlider(Graphics&,
                          int x,
                          int y,
                          int width,
                          int height,
                          float sliderPosProportional,
                          float rotaryStartAngle,
                          float rotaryEndAngle,
                          Slider&) override;

    void drawTooltip(Graphics&, const String&, int, int) override;
};

#endif // NeuralNoteLNF_h
