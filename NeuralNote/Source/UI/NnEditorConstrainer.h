//
// Created by Damien Ronssin on 20.08.26.
//

#ifndef NnEditorConstrainer_h
#define NnEditorConstrainer_h

#include <JuceHeader.h>

/**
 * Bounds constrainer for the editor, capping it to what fits on the display it is currently on.
 *
 * Installing it with setConstrainer() is what makes the cap native: the host and the OS consult
 * checkBounds() during their own resize negotiation, so the container window itself cannot be
 * dragged past the screen. A reactive setSize() cannot do that -- it only shrinks the editor and
 * leaves the host window oversized.
 *
 * The maximum is recomputed on every check from the display the editor sits on, so it follows the
 * window across monitors. userBounds is the usable area, menu bar and dock/taskbar excluded.
 */
class NnEditorConstrainer : public juce::ComponentBoundsConstrainer
{
public:
    /** @param inEditor The editor this governs, whose display is queried on every check. */
    void configure(juce::Component* inEditor);

    /** @return The largest scale the current display allows, never above nn::metrics::maxEditorScale. */
    double maxScaleForCurrentDisplay() const;

    /** @return inScale brought inside the allowed range. Applied on every path the factor arrives by. */
    double clampScale(double inScale) const;

    /**
     * Called when a resize drag finishes, for the editor to persist the size it ended at.
     *
     * Not every drag reaches here: the standalone's window wraps this constrainer in JUCE's own
     * DecoratorConstrainer, which forwards checkBounds but not resizeEnd, and asks for no corner
     * resizer. So this fires in a plugin host and not in the standalone.
     */
    std::function<void()> onResizeEnd;

    void checkBounds(juce::Rectangle<int>& ioBounds,
                     const juce::Rectangle<int>& inPreviousBounds,
                     const juce::Rectangle<int>& inLimits,
                     bool inIsStretchingTop,
                     bool inIsStretchingLeft,
                     bool inIsStretchingBottom,
                     bool inIsStretchingRight) override;

    void resizeEnd() override;

private:
    void _applyMaximumSizeForCurrentDisplay();

    // Of the display's usable area. Height leaves more room because what surrounds the editor
    // stacks vertically: the standalone's title bar and feedback-loop banner, a host's frame.
    static constexpr double MAX_DISPLAY_PERCENT_WIDTH = 0.99;
    static constexpr double MAX_DISPLAY_PERCENT_HEIGHT = 0.90;

    juce::Component* mEditor = nullptr;

    JUCE_LEAK_DETECTOR(NnEditorConstrainer)
};

#endif // NnEditorConstrainer_h
