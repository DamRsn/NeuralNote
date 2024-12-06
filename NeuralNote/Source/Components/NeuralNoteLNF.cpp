//
// Created by Damien Ronssin on 17.03.23.
//

#include "NeuralNoteLNF.h"

NeuralNoteLNF::NeuralNoteLNF()
{
    setColour(juce::ComboBox::backgroundColourId, WHITE_TRANSPARENT);
    setColour(juce::ComboBox::textColourId, BLACK);
    setColour(juce::ComboBox::arrowColourId, BLACK);

    setColour(juce::PopupMenu::textColourId, BLACK);
    setColour(juce::PopupMenu::backgroundColourId, WHITE_SOLID);
    setColour(juce::PopupMenu::highlightedTextColourId, WHITE_SOLID);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, BLACK);

    setColour(juce::Slider::ColourIds::thumbColourId, juce::Colours::white);
    setColour(juce::Slider::ColourIds::backgroundColourId, KNOB_GREY);
    setColour(juce::Slider::ColourIds::trackColourId, BLACK);
}

void NeuralNoteLNF::drawRotarySlider(Graphics& g,
                                     int x,
                                     int y,
                                     int width,
                                     int height,
                                     float sliderPosProportional,
                                     float rotaryStartAngle,
                                     float rotaryEndAngle,
                                     Slider& slider)
{
    auto bounds = Rectangle<int>(x, y, width, height).toFloat();

    auto out_radius = jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto toAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    auto lineW = 8.0f;
    jmin(8.0f, out_radius * 0.5f);
    auto arcRadius = out_radius - lineW * 0.5f;

    Path backgroundArc;
    backgroundArc.addCentredArc(
        bounds.getCentreX(), bounds.getCentreY(), arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);

    g.setColour(KNOB_GREY);
    g.strokePath(backgroundArc, PathStrokeType(lineW, PathStrokeType::curved, PathStrokeType::rounded));

    Path valueArc;
    valueArc.addCentredArc(
        bounds.getCentreX(), bounds.getCentreY(), arcRadius, arcRadius, 0.0f, rotaryStartAngle, toAngle, true);

    g.setColour(BLACK);
    g.strokePath(valueArc, PathStrokeType(lineW, PathStrokeType::curved, PathStrokeType::rounded));

    auto thumbWidth = 5.0f;
    Point<float> thumbPoint(bounds.getCentreX() + 10 * std::cos(toAngle - MathConstants<float>::halfPi),
                            bounds.getCentreY() + 10 * std::sin(toAngle - MathConstants<float>::halfPi));

    g.setColour(BLACK);
    g.fillEllipse(Rectangle<float>(thumbWidth, thumbWidth).withCentre(thumbPoint));

    g.drawEllipse(bounds.getCentreX() - 16, bounds.getCentreY() - 16, 32, 32, 0.5f);
}

void NeuralNoteLNF::drawTooltip(Graphics& g, const String& text, int width, int height)
{
    g.setColour(WHITE_SOLID.darker(0.0f));
    g.fillRoundedRectangle(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 5.0f);

    g.setColour(BLACK);
    g.setFont(LABEL_FONT);

    auto text_std = text.toStdString();
    int num_line_breaks = static_cast<int>(std::count(text_std.begin(), text_std.end(), '\n'));

    g.drawFittedText(text, 0, 0, width, height, Justification::centred, num_line_breaks + 1);
}
