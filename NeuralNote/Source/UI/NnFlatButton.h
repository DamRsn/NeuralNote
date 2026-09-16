//
// Created by Damien Ronssin on 07.08.26.
//

#ifndef NnFlatButton_h
#define NnFlatButton_h

#include <functional>

#include <JuceHeader.h>

#include "NnLook.h"

/**
 * The one button in the v2 UI: a flat rounded rectangle carrying an icon, a label, or both.
 *
 * Covers the transport row, the toolbar, the instrument strips' M/S pair, the sidebar's "+" and
 * the settings button -- they differ only in colours, corner radius and content, so they are set
 * through setColour rather than expressed as subclasses or a LookAndFeel.
 *
 * Hover, pressed and disabled are never separate colour ids; they are derived from the idle and
 * "on" colours by nn::surfaceFor / nn::foregroundFor, so every button in the window reacts alike.
 */
class NnFlatButton : public juce::Button
{
public:
    enum ColourIds {
        backgroundColourId = 0x6e9b100,
        backgroundOnColourId = 0x6e9b101,
        iconColourId = 0x6e9b102,
        iconOnColourId = 0x6e9b103,
        textColourId = 0x6e9b104,
        textOnColourId = 0x6e9b105,
        outlineColourId = 0x6e9b106,
        outlineOnColourId = 0x6e9b107,
    };

    enum class IconStyle { filled, stroked };

    using IconBuilder = std::function<juce::Path(juce::Rectangle<float>)>;

    explicit NnFlatButton(const juce::String& inName);

    /** @param inSizePx Side of the square the icon is drawn in. */
    void setIcon(IconBuilder inBuilder, IconStyle inStyle, float inSizePx);

    /**
     * A second path filled over the icon in the same colour, for the one icon that mixes a stroked
     * outline with a solid interior (the loop button's play head).
     */
    void setOverlayIcon(IconBuilder inBuilder);

    void setLabel(const juce::String& inText, juce::Font inFont, float inTrackingEm = 0.0f);

    void setCornerRadius(float inRadius);

    /** Horizontal padding, and the gap between the icon and the label. */
    void setPadding(int inLeft, int inRight, int inIconLabelGap);

    /** @return The width the current icon, gap, label and padding need. */
    int getIdealWidth() const;

    /**
     * Called instead of onClick for a right-click (or a ctrl-click on macOS), for a button that
     * offers a menu of variants alongside its plain action. Unset on all but the few that do, and
     * those keep their left-click behaviour unchanged.
     */
    std::function<void()> onRightClick;

    void paintButton(juce::Graphics& g, bool inIsHighlighted, bool inIsDown) override;

    void enablementChanged() override;

    void mouseDown(const juce::MouseEvent& inEvent) override;

private:
    IconBuilder mIconBuilder;
    IconBuilder mOverlayIconBuilder;
    IconStyle mIconStyle = IconStyle::filled;
    float mIconSize = 14.0f;

    juce::String mLabel;
    juce::Font mLabelFont {juce::FontOptions {}};
    float mLabelTracking = 0.0f;

    float mCornerRadius = static_cast<float>(nn::metrics::controlCorner);
    int mPaddingLeft = 0;
    int mPaddingRight = 0;
    int mIconLabelGap = 7;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NnFlatButton)
};

#endif // NnFlatButton_h
