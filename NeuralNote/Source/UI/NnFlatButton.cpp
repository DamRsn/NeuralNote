//
// Created by Damien Ronssin on 07.08.26.
//

#include "NnFlatButton.h"

#include "NnIcons.h"

NnFlatButton::NnFlatButton(const juce::String& inName)
    : juce::Button(inName)
{
    setColour(backgroundColourId, juce::Colours::transparentBlack);
    setColour(backgroundOnColourId, nn::colours::bgControlActive);
    setColour(iconColourId, nn::colours::textIcon);
    setColour(iconOnColourId, nn::colours::textPrimary);
    setColour(textColourId, nn::colours::textButton);
    setColour(textOnColourId, nn::colours::textBright);
    setColour(outlineColourId, juce::Colours::transparentBlack);
    setColour(outlineOnColourId, juce::Colours::transparentBlack);

    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void NnFlatButton::setIcon(IconBuilder inBuilder, IconStyle inStyle, float inSizePx)
{
    mIconBuilder = std::move(inBuilder);
    mIconStyle = inStyle;
    mIconSize = inSizePx;
    repaint();
}

void NnFlatButton::setOverlayIcon(IconBuilder inBuilder)
{
    mOverlayIconBuilder = std::move(inBuilder);
    repaint();
}

void NnFlatButton::setLabel(const juce::String& inText, juce::Font inFont, float inTrackingEm)
{
    mLabel = inText;
    mLabelFont = std::move(inFont);
    mLabelTracking = inTrackingEm;
    repaint();
}

void NnFlatButton::setCornerRadius(float inRadius)
{
    mCornerRadius = inRadius;
    repaint();
}

void NnFlatButton::setPadding(int inLeft, int inRight, int inIconLabelGap)
{
    mPaddingLeft = inLeft;
    mPaddingRight = inRight;
    mIconLabelGap = inIconLabelGap;
    resized();
    repaint();
}

int NnFlatButton::getIdealWidth() const
{
    int width = mPaddingLeft + mPaddingRight;

    if (mIconBuilder != nullptr)
        width += juce::roundToInt(mIconSize);

    if (mLabel.isNotEmpty()) {
        if (mIconBuilder != nullptr)
            width += mIconLabelGap;

        width += juce::roundToInt(std::ceil(nn::trackedTextWidth(mLabel, mLabelFont, mLabelTracking)));
    }

    return width;
}

void NnFlatButton::mouseDown(const juce::MouseEvent& inEvent)
{
    // Swallowed rather than passed on: letting Button see it would arm the press, and the button
    // would fire its plain action when the menu closed.
    if (isEnabled() && onRightClick != nullptr && inEvent.mods.isPopupMenu()) {
        onRightClick();
        return;
    }

    juce::Button::mouseDown(inEvent);
}

void NnFlatButton::enablementChanged()
{
    setMouseCursor(isEnabled() ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
    repaint();
}

void NnFlatButton::paintButton(juce::Graphics& g, bool inIsHighlighted, bool inIsDown)
{
    const nn::ControlState state {inIsHighlighted, inIsDown, getToggleState(), isEnabled()};

    // Multiplied into each colour rather than pushed as a transparency layer: a layer is an
    // offscreen buffer per repaint, and this is the most frequently painted widget in the window.
    const float alpha = state.isEnabled ? 1.0f : nn::DISABLED_ALPHA;

    const auto bounds = getLocalBounds().toFloat();

    const juce::Colour background =
        nn::surfaceFor(findColour(backgroundColourId), findColour(backgroundOnColourId), state);

    if (!background.isTransparent()) {
        g.setColour(background.withMultipliedAlpha(alpha));
        g.fillRoundedRectangle(bounds, mCornerRadius);
    }

    const juce::Colour outline = state.isOn ? findColour(outlineOnColourId) : findColour(outlineColourId);

    if (!outline.isTransparent()) {
        g.setColour(outline.withMultipliedAlpha(alpha));
        g.drawRoundedRectangle(bounds.reduced(0.5f), mCornerRadius, 1.0f);
    }

    // The icon and the label share one row, centred as a unit, so an icon-only button and an
    // icon-plus-label button of the same width put their icons in different places -- as intended.
    auto content = getLocalBounds();
    content.removeFromLeft(mPaddingLeft);
    content.removeFromRight(mPaddingRight);

    const int label_width =
        mLabel.isEmpty() ? 0 : juce::roundToInt(std::ceil(nn::trackedTextWidth(mLabel, mLabelFont, mLabelTracking)));
    const int icon_width = mIconBuilder != nullptr ? juce::roundToInt(mIconSize) : 0;
    const int gap = (icon_width > 0 && label_width > 0) ? mIconLabelGap : 0;
    const int content_width = icon_width + gap + label_width;

    // Placed in floats: an odd-sized icon in an even-sized button lands half a pixel off centre if
    // the arithmetic rounds here, which is visible on a small glyph like the cancel cross.
    float x = static_cast<float>(content.getCentreX()) - static_cast<float>(content_width) / 2.0f;

    if (mIconBuilder != nullptr) {
        const juce::Colour icon_colour = nn::foregroundFor(findColour(iconColourId), findColour(iconOnColourId), state);

        const auto icon_box = juce::Rectangle<float>(
            x, static_cast<float>(content.getCentreY()) - mIconSize / 2.0f, mIconSize, mIconSize);

        g.setColour(icon_colour.withMultipliedAlpha(alpha));

        const juce::Path icon = mIconBuilder(icon_box);

        if (mIconStyle == IconStyle::stroked)
            g.strokePath(icon,
                         juce::PathStrokeType(
                             nn::icons::STROKE_WIDTH, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        else
            g.fillPath(icon);

        if (mOverlayIconBuilder != nullptr)
            g.fillPath(mOverlayIconBuilder(icon_box));

        x += static_cast<float>(icon_width + gap);
    }

    if (mLabel.isNotEmpty()) {
        const juce::Colour text_colour = nn::foregroundFor(findColour(textColourId), findColour(textOnColourId), state);

        g.setColour(text_colour.withMultipliedAlpha(alpha));
        nn::drawTrackedText(g,
                            mLabel,
                            mLabelFont,
                            juce::Rectangle<float>(x,
                                                   static_cast<float>(content.getY()),
                                                   static_cast<float>(label_width),
                                                   static_cast<float>(content.getHeight())),
                            juce::Justification::centredLeft,
                            mLabelTracking);
    }
}
