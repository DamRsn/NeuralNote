//
// Created by Damien Ronssin on 07.08.26.
//

#include "NnFlatSlider.h"

#include "NnLook.h"

int NnFlatSlider::ThumbLayout::getSliderThumbRadius(juce::Slider&)
{
    return THUMB_INDENT;
}

NnFlatSlider::NnFlatSlider()
    : juce::Slider(juce::Slider::LinearHorizontal, juce::Slider::NoTextBox)
{
    setLookAndFeel(&mThumbLayout.get());

    setColour(trackColourId, nn::colours::faderTrack);
    setColour(fillColourId, nn::colours::accent);
    setColour(thumbColourId, nn::colours::faderThumb);

    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

NnFlatSlider::~NnFlatSlider()
{
    // Before ~Slider, which would otherwise outlive the look and feel it still points at.
    setLookAndFeel(nullptr);
}

void NnFlatSlider::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const float alpha = isEnabled() ? 1.0f : nn::DISABLED_ALPHA;

    // The same travel juce::Slider maps the drag across, so the thumb sits under the cursor.
    const float indent = static_cast<float>(THUMB_INDENT);
    const float travel = juce::jmax(1.0f, bounds.getWidth() - 2.0f * indent);
    const float proportion = static_cast<float>(valueToProportionOfLength(getValue()));
    const float thumb_centre = bounds.getX() + indent + proportion * travel;

    const auto track = juce::Rectangle<float>(bounds.getX(), 0.0f, bounds.getWidth(), TRACK_HEIGHT)
                           .withCentre({bounds.getCentreX(), bounds.getCentreY()});

    g.setColour(findColour(trackColourId).withMultipliedAlpha(alpha));
    g.fillRoundedRectangle(track, TRACK_HEIGHT / 2.0f);

    const float fill_width = thumb_centre - track.getX();

    if (fill_width > 0.0f) {
        g.setColour(findColour(fillColourId).withMultipliedAlpha(alpha));
        g.fillRoundedRectangle(track.withWidth(fill_width), TRACK_HEIGHT / 2.0f);
    }

    g.setColour(findColour(thumbColourId).withMultipliedAlpha(alpha));
    g.fillRoundedRectangle(
        juce::Rectangle<float>(THUMB_WIDTH, THUMB_HEIGHT).withCentre({thumb_centre, bounds.getCentreY()}),
        THUMB_WIDTH / 2.0f);
}
