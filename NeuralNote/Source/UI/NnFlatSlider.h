//
// Created by Damien Ronssin on 07.08.26.
//

#ifndef NnFlatSlider_h
#define NnFlatSlider_h

#include <JuceHeader.h>

/**
 * The flat horizontal fader used by the dry/wet control, the output volume and every instrument
 * strip: a 3 px track, a fill up to the value, and a 3 x 11 thumb.
 *
 * Subclasses juce::Slider and overrides only paint(), so dragging, the value range, double-click
 * to reset and SliderParameterAttachment all keep working -- the LookAndFeel is bypassed rather
 * than replaced.
 *
 * With one exception. juce::Slider takes the pixel range it maps values across from
 * LookAndFeel::getSliderLayout, which insets it by getSliderThumbRadius -- a thumb far wider than
 * this one, derived from the slider's height. Painting the thumb over a narrower travel than the
 * drag maps across leaves the two out of step, so the one LookAndFeel method that number comes
 * from is overridden below and both sides read THUMB_INDENT.
 */
class NnFlatSlider : public juce::Slider
{
public:
    enum ColourIds {
        trackColourId = 0x6e9b200,
        fillColourId = 0x6e9b201,
        thumbColourId = 0x6e9b202,
    };

    NnFlatSlider();

    ~NnFlatSlider() override;

    void paint(juce::Graphics& g) override;

private:
    /** Only overrides the thumb radius the slider's drag region is derived from. */
    class ThumbLayout : public juce::LookAndFeel_V4
    {
    public:
        int getSliderThumbRadius(juce::Slider&) override;
    };

    static constexpr float TRACK_HEIGHT = 3.0f;
    static constexpr float THUMB_WIDTH = 3.0f;
    static constexpr float THUMB_HEIGHT = 11.0f;

    // Half the thumb, rounded up: the drag region is inset by whole pixels, so a travel of
    // THUMB_WIDTH / 2 could not be expressed exactly on both sides.
    static constexpr int THUMB_INDENT = 2;

    // One instance shared by every fader, released with the last of them.
    juce::SharedResourcePointer<ThumbLayout> mThumbLayout;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NnFlatSlider)
};

#endif // NnFlatSlider_h
