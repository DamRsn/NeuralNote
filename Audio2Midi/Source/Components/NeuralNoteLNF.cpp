//
// Created by Damien Ronssin on 17.03.23.
//

#include "NeuralNoteLNF.h"

NeuralNoteLNF::NeuralNoteLNF()
{
    setDefaultSansSerifTypeface(MONTSERRAT_REGULAR);

    setColour(juce::ComboBox::backgroundColourId, WHITE_TRANSPARENT);
    setColour(juce::ComboBox::textColourId, FONT_BLACK);
    setColour(juce::ComboBox::arrowColourId, FONT_BLACK);

    setColour(juce::PopupMenu::textColourId, FONT_BLACK);
    setColour(juce::PopupMenu::backgroundColourId, WHITE_SOLID);
    setColour(juce::PopupMenu::highlightedTextColourId, WHITE_SOLID);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, PINK);

    setColour(juce::Slider::ColourIds::thumbColourId, PINK);
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
    auto outline = slider.findColour(Slider::rotarySliderOutlineColourId);
    auto fill = slider.findColour(Slider::rotarySliderFillColourId);

    auto bounds = Rectangle<int>(x, y, width, height).toFloat().reduced(10);

    auto radius = jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto toAngle =
        rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    auto lineW = jmin(8.0f, radius * 0.5f);
    auto arcRadius = radius - lineW * 0.5f;

    Path backgroundArc;
    backgroundArc.addCentredArc(bounds.getCentreX(),
                                bounds.getCentreY(),
                                arcRadius,
                                arcRadius,
                                0.0f,
                                rotaryStartAngle,
                                rotaryEndAngle,
                                true);

    g.setColour(PINK);
    g.strokePath(backgroundArc,
                 PathStrokeType(lineW, PathStrokeType::curved, PathStrokeType::rounded));

    if (slider.isEnabled())
    {
        Path valueArc;
        valueArc.addCentredArc(bounds.getCentreX(),
                               bounds.getCentreY(),
                               arcRadius,
                               arcRadius,
                               0.0f,
                               rotaryStartAngle,
                               toAngle,
                               true);

        g.setColour(fill);
        g.strokePath(
            valueArc,
            PathStrokeType(lineW, PathStrokeType::curved, PathStrokeType::rounded));
    }

//    auto thumbWidth = lineW * 2.0f;
//    Point<float> thumbPoint(
//        bounds.getCentreX()
//            + arcRadius * std::cos(toAngle - MathConstants<float>::halfPi),
//        bounds.getCentreY()
//            + arcRadius * std::sin(toAngle - MathConstants<float>::halfPi));
//
//    g.setColour(slider.findColour(Slider::thumbColourId));
//    g.fillEllipse(Rectangle<float>(thumbWidth, thumbWidth).withCentre(thumbPoint));
}
