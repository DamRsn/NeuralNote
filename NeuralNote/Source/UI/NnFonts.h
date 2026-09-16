//
// Created by Damien Ronssin on 07.08.26.
//

#ifndef NnFonts_h
#define NnFonts_h

#include <JuceHeader.h>

/**
 * The two bundled families and the type scale built on them.
 *
 * The rule the design runs on: anything the user reads as data -- times, dB values, note counts,
 * tempo, key labels -- is JetBrains Mono; anything that is a label or a name is Inter.
 *
 * Sizes are em sizes, so they match the mockup's CSS pixel sizes directly (withPointHeight, not
 * withHeight). Every accessor returns a cached Font: constructing one is not free and none of
 * these belong inside a paint().
 */
namespace nn::fonts
{
juce::Font sans(float inSizePx, int inWeight);

juce::Font mono(float inSizePx, int inWeight);

// ---- the type scale, size / weight ---------------------------------------
juce::Font wordmark(); //  15 / 600, tracking 0.14em
juce::Font wordmarkVersion(); //   9 mono / 500
juce::Font transportTime(); //  15 mono / 500
juce::Font transportTotal(); //  11 mono / 400
juce::Font filename(); // 12.5 / 500
juce::Font instrumentName(); //  12 / 500
juce::Font buttonLabel(); // 11.5 / 500
juce::Font menuItem(); // 11.5 / 400
juce::Font menuItemTicked(); // 11.5 / 500
juce::Font tempoValue(); // 11.5 mono / 400
juce::Font sectionHeader(); //  10 / 600, tracking 0.13em
juce::Font pillLabel(); // 9.5 / 500, tracking 0.08em
juce::Font statusBar(); // 9.5 mono / 400
juce::Font meta(); //   9 mono / 400
juce::Font metaStrong(); //   9 mono / 600
juce::Font scaleLabel(); // 7.5 mono / 400
} // namespace nn::fonts

#endif // NnFonts_h
