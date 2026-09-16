//
// Created by Damien Ronssin on 07.08.26.
//

#include "NnLook.h"

#include "NnFonts.h"
#include "NnIcons.h"

namespace nn
{
namespace
{
    /** Tracking in pixels. The em size is what CSS resolves an "em" against, not the line height. */
    float trackingPixels(const juce::Font& inFont, float inTrackingEm)
    {
        return inTrackingEm * inFont.getHeightInPoints();
    }
} // namespace

float trackedTextWidth(const juce::String& inText, const juce::Font& inFont, float inTrackingEm)
{
    const float natural = juce::GlyphArrangement::getStringWidth(inFont, inText);
    const int num_gaps = juce::jmax(0, inText.length() - 1);

    return natural + static_cast<float>(num_gaps) * trackingPixels(inFont, inTrackingEm);
}

void drawTrackedText(juce::Graphics& g,
                     const juce::String& inText,
                     const juce::Font& inFont,
                     juce::Rectangle<float> inArea,
                     juce::Justification inJustification,
                     float inTrackingEm)
{
    if (inText.isEmpty())
        return;

    juce::GlyphArrangement arrangement;
    arrangement.addLineOfText(inFont, inText, 0.0f, 0.0f);

    const int num_glyphs = arrangement.getNumGlyphs();

    if (num_glyphs == 0)
        return;

    // Spread the glyphs first, then place the result: the justification has to see the tracked
    // width, or a right-aligned label drifts by the whole accumulated spacing.
    const float tracking = trackingPixels(inFont, inTrackingEm);

    for (int i = 1; i < num_glyphs; ++i) {
        arrangement.getGlyph(i).moveBy(static_cast<float>(i) * tracking, 0.0f);
    }

    const juce::Rectangle<float> bounds = arrangement.getBoundingBox(0, num_glyphs, true);

    float x = inArea.getX();

    if (inJustification.testFlags(juce::Justification::horizontallyCentred))
        x = inArea.getCentreX() - bounds.getWidth() / 2.0f;
    else if (inJustification.testFlags(juce::Justification::right))
        x = inArea.getRight() - bounds.getWidth();

    // Baselines, not bounding boxes: two labels of different weights on the same row have
    // different bounding-box tops and would not line up if centred by box.
    float baseline = inArea.getY() + inFont.getAscent();

    if (inJustification.testFlags(juce::Justification::verticallyCentred))
        baseline = inArea.getCentreY() - (inFont.getAscent() + inFont.getDescent()) / 2.0f + inFont.getAscent();
    else if (inJustification.testFlags(juce::Justification::bottom))
        baseline = inArea.getBottom() - inFont.getDescent();

    // Against the bounding box's own left edge, not the first glyph's origin: a glyph with a left
    // side bearing would otherwise sit a fraction of a pixel off from a plain drawText beside it.
    arrangement.draw(g, juce::AffineTransform::translation(x - bounds.getX(), baseline));
}

juce::String midiNoteName(int inMidiNote)
{
    static const char* names[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

    const int note = ((inMidiNote % 12) + 12) % 12;
    const int octave = inMidiNote / 12 - 1;

    return juce::String(names[note]) + juce::String(octave);
}

void drawPopupSurface(juce::Graphics& g, juce::Rectangle<float> inBounds)
{
    g.setColour(nn::colours::popupBg);
    g.fillRoundedRectangle(inBounds, nn::metrics::menuCorner);

    g.setColour(nn::colours::popupBorder);
    g.drawRoundedRectangle(inBounds.reduced(0.5f), nn::metrics::menuCorner, 1.0f);
}

void drawCheckbox(juce::Graphics& g, juce::Rectangle<int> inArea, bool inIsTicked, float inAlpha)
{
    const auto box =
        inArea.withSizeKeepingCentre(nn::metrics::checkboxSize, nn::metrics::checkboxSize).toFloat().reduced(0.5f);

    if (inIsTicked) {
        g.setColour(nn::colours::accent.withMultipliedAlpha(inAlpha));
        g.fillRoundedRectangle(box, nn::metrics::checkboxCorner);

        g.setColour(nn::colours::checkboxTick.withMultipliedAlpha(inAlpha));
        g.strokePath(nn::icons::checkStroked(box),
                     juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    } else {
        g.setColour(nn::colours::checkboxBorder.withMultipliedAlpha(inAlpha));
        g.drawRoundedRectangle(box, nn::metrics::checkboxCorner, 1.0f);
    }
}
} // namespace nn

namespace
{
/** Room for the tick box and the padding either side of it, at the right-hand end of a row. */
constexpr int MENU_TICK_COLUMN = nn::metrics::checkboxSize + nn::metrics::menuPadX;

constexpr int MENU_SEPARATOR_HEIGHT = 9;
constexpr int MENU_MIN_WIDTH = 180;

/** Where a tooltip wraps. Two of ours are already two lines, and one long line reads worse. */
constexpr int TOOLTIP_MAX_WIDTH = 260;
constexpr int TOOLTIP_PAD_Y = 6;

/** Laid out once for the bounds and again for the drawing, so the two cannot disagree. */
juce::TextLayout tooltipLayout(const juce::String& inText)
{
    juce::AttributedString attributed;
    attributed.setJustification(juce::Justification::centredLeft);
    attributed.append(inText, nn::fonts::menuItem(), nn::colours::popupItem);

    juce::TextLayout layout;
    layout.createLayout(attributed, static_cast<float>(TOOLTIP_MAX_WIDTH));

    return layout;
}
} // namespace

nn::NeuralNoteLookAndFeel::NeuralNoteLookAndFeel()
{
    // Seeds every colour id the settings dialog's widgets read -- combo boxes, labels, buttons,
    // tick boxes, the MIDI input list and its scrollbar -- from nine values, rather than naming
    // each id here and finding out which ones were missed by looking at the dialog.
    setColourScheme({nn::colours::bgRoot,
                     nn::colours::bgControl,
                     nn::colours::popupBg,
                     nn::colours::divStrong,
                     nn::colours::textPrimary,
                     nn::colours::accent,
                     nn::colours::textBright,
                     nn::colours::accent,
                     nn::colours::popupItem});

    // After the scheme, not before: setColourScheme re-initialises every colour, these included.
    setColour(juce::PopupMenu::backgroundColourId, nn::colours::popupBg);
    setColour(juce::PopupMenu::textColourId, nn::colours::popupItem);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, nn::colours::popupRowHover);
    setColour(juce::PopupMenu::highlightedTextColourId, nn::colours::popupItemTicked);
}

juce::Font nn::NeuralNoteLookAndFeel::getPopupMenuFont()
{
    return nn::fonts::menuItem();
}

int nn::NeuralNoteLookAndFeel::getPopupMenuBorderSizeWithOptions(const juce::PopupMenu::Options& inOptions)
{
    juce::ignoreUnused(inOptions);

    return nn::metrics::menuListPadY;
}

void nn::NeuralNoteLookAndFeel::drawPopupMenuBackgroundWithOptions(juce::Graphics& g,
                                                                  int inWidth,
                                                                  int inHeight,
                                                                  const juce::PopupMenu::Options& inOptions)
{
    juce::ignoreUnused(inOptions);

    nn::drawPopupSurface(
        g, juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(inWidth), static_cast<float>(inHeight)));
}

void nn::NeuralNoteLookAndFeel::getIdealPopupMenuItemSizeWithOptions(const juce::String& inText,
                                                                    bool inIsSeparator,
                                                                    int inStandardItemHeight,
                                                                    int& outIdealWidth,
                                                                    int& outIdealHeight,
                                                                    const juce::PopupMenu::Options& inOptions)
{
    juce::ignoreUnused(inStandardItemHeight, inOptions);

    if (inIsSeparator) {
        outIdealHeight = MENU_SEPARATOR_HEIGHT;
        outIdealWidth = MENU_MIN_WIDTH;
        return;
    }

    outIdealHeight = nn::metrics::menuRowHeight;
    outIdealWidth = juce::jmax(MENU_MIN_WIDTH,
                               juce::GlyphArrangement::getStringWidthInt(getPopupMenuFont(), inText)
                                   + nn::metrics::menuPadX + MENU_TICK_COLUMN + nn::metrics::menuPadX);
}

void nn::NeuralNoteLookAndFeel::drawPopupMenuItemWithOptions(juce::Graphics& g,
                                                            const juce::Rectangle<int>& inArea,
                                                            bool inIsHighlighted,
                                                            const juce::PopupMenu::Item& inItem,
                                                            const juce::PopupMenu::Options& inOptions)
{
    juce::ignoreUnused(inOptions);

    if (inItem.isSeparator) {
        g.setColour(nn::colours::divStrong);
        g.fillRect(inArea.withSizeKeepingCentre(inArea.getWidth(), 1));
        return;
    }

    if (inIsHighlighted && inItem.isEnabled) {
        g.setColour(nn::colours::popupRowHover);
        g.fillRect(inArea);
    }

    auto row = inArea.reduced(nn::metrics::menuPadX, 0);

    // The tick sits at the right-hand end, where the instrument dropdown puts it, rather than in
    // JUCE's left gutter -- which would leave every unticked row's label indented past nothing.
    const auto tick_area = row.removeFromRight(nn::metrics::checkboxSize);

    const float alpha = inItem.isEnabled ? 1.0f : nn::DISABLED_ALPHA;

    // Nothing is drawn for an unticked row: a menu item here is as often an action as a toggle, and
    // an empty box beside "Check for updates" would promise a state it does not have.
    if (inItem.isTicked) {
        nn::drawCheckbox(g, tick_area, true, alpha);
    }

    g.setColour((inItem.isTicked ? nn::colours::popupItemTicked : nn::colours::popupItem).withMultipliedAlpha(alpha));
    g.setFont(inItem.isTicked ? nn::fonts::menuItemTicked() : getPopupMenuFont());
    g.drawText(inItem.text, row.withTrimmedRight(nn::metrics::menuPadX), juce::Justification::centredLeft, true);
}

juce::Rectangle<int> nn::NeuralNoteLookAndFeel::getTooltipBounds(const juce::String& inTipText,
                                                                juce::Point<int> inScreenPos,
                                                                juce::Rectangle<int> inParentArea)
{
    const juce::TextLayout layout = tooltipLayout(inTipText);

    // Rounded up: half a pixel short of the laid-out width is a wrap that was not asked for.
    const int width = juce::roundToInt(std::ceil(layout.getWidth())) + 2 * nn::metrics::menuPadX;
    const int height = juce::roundToInt(std::ceil(layout.getHeight())) + 2 * TOOLTIP_PAD_Y;

    // JUCE's own placement, kept: the tooltip goes away from whichever edge the pointer is nearest,
    // so it never lands on top of the thing being hovered.
    return juce::Rectangle<int>(
               inScreenPos.x > inParentArea.getCentreX() ? inScreenPos.x - (width + 12) : inScreenPos.x + 24,
               inScreenPos.y > inParentArea.getCentreY() ? inScreenPos.y - (height + 6) : inScreenPos.y + 6,
               width,
               height)
        .constrainedWithin(inParentArea);
}

void nn::NeuralNoteLookAndFeel::drawTooltip(juce::Graphics& g, const juce::String& inText, int inWidth, int inHeight)
{
    // Not fillAll: the window is a transparent child of the editor, so it can round its corners the
    // way the menus do instead of being the one square panel in the window.
    const auto bounds = juce::Rectangle<int>(inWidth, inHeight).toFloat();

    nn::drawPopupSurface(g, bounds);

    tooltipLayout(inText).draw(g, bounds.reduced(nn::metrics::menuPadX, TOOLTIP_PAD_Y));
}

void nn::NeuralNoteLookAndFeel::drawCornerResizer(juce::Graphics& g,
                                                  int inWidth,
                                                  int inHeight,
                                                  bool inIsMouseOver,
                                                  bool inIsMouseDragging)
{
    const auto colour = (inIsMouseOver || inIsMouseDragging) ? nn::colours::textDim : nn::colours::textScale;

    g.setColour(colour);

    const auto size = (float) juce::jmin(inWidth, inHeight);

    for (float offset: {0.30f, 0.55f, 0.80f}) {
        const float from = size * offset;
        g.drawLine(size, from, from, size, 1.0f);
    }
}
