//
// Created by Damien Ronssin on 07.08.26.
//

#ifndef NnLook_h
#define NnLook_h

#include <JuceHeader.h>

/**
 * NeuralNote v2 palette, metrics and the shared rules for interaction states.
 *
 * Our own widgets deliberately do not go through a LookAndFeel: each keeps its own colour enum,
 * seeded from the constants here, so what a widget draws is readable from the widget. What a
 * LookAndFeel would otherwise centralise -- and what genuinely has to be consistent -- lives here
 * instead: the palette, the fixed extents, and the hover/pressed/disabled rules below.
 *
 * NeuralNoteLookAndFeel at the bottom is the exception, and only covers what we do not draw
 * ourselves: the surfaces JUCE draws on our behalf.
 */
namespace nn::colours
{
inline juce::Colour rgb(juce::uint32 hex)
{
    return juce::Colour(hex).withAlpha(1.0f);
}

// ---- surfaces: light studio / soft glass --------------------------------
inline const juce::Colour windowBorder = rgb(0xC7D2E3);
inline const juce::Colour bgRoot = rgb(0xE5ECF6);
inline const juce::Colour bgTopBar = rgb(0xF4F7FC);
inline const juce::Colour bgSidebar = rgb(0xF3F7FD);
inline const juce::Colour bgPanel = rgb(0xF8FAFE);
inline const juce::Colour bgGutter = rgb(0xE4EBF5);
inline const juce::Colour bgControl = rgb(0xFFFFFF);
inline const juce::Colour bgControlAlt = rgb(0xE8EFF8);
inline const juce::Colour bgControlSubtle = rgb(0xDDE7F4);
inline const juce::Colour bgControlActive = rgb(0xD0E2F8);

// ---- dividers -----------------------------------------------------------
inline const juce::Colour divStrong = rgb(0xC9D5E5); // top bar / sidebar edges
inline const juce::Colour divSoft = rgb(0xD6E0ED); // section separators
inline const juce::Colour divRow = rgb(0xE1E8F2); // between instrument strips
inline const juce::Colour divTick = rgb(0xB6C7DA); // ruler ticks
inline const juce::Colour divPitch = rgb(0xE5ECF5); // subtle semitone boundaries in the piano roll
inline const juce::Colour divOctave = rgb(0xC3D2E4); // stronger C-row separator in the grid

// ---- text ---------------------------------------------------------------
inline const juce::Colour textBright = rgb(0x17283E); // wordmark, transport time, playhead
inline const juce::Colour textPrimary = rgb(0x26364D); // default
inline const juce::Colour textStrong = rgb(0x1F3148); // instrument names
inline const juce::Colour textFile = rgb(0x33465F); // filename, tempo value
inline const juce::Colour textButton = rgb(0x43566F); // toolbar button labels
inline const juce::Colour textIcon = rgb(0x657A95); // idle icons
inline const juce::Colour textIconSoft = rgb(0x8295AD);
inline const juce::Colour textLabel = rgb(0x71839A); // section headers, muted names
inline const juce::Colour textDim = rgb(0x6B7D94); // dB values, M/S off
inline const juce::Colour textFaint = rgb(0x8496AD); // total duration, ruler labels
inline const juce::Colour textFainter = rgb(0x8496AA); // status bar, meta lines
inline const juce::Colour textScale = rgb(0x91A2B8); // amplitude scale, corner label
inline const juce::Colour textSeparator = rgb(0xA0B1C6); // status bar separator dot

// ---- accent: restrained studio cyan -------------------------------------
inline const juce::Colour accent = rgb(0x398DF3);
inline const juce::Colour accentText = rgb(0xFFFFFF);
inline const juce::Colour warn = rgb(0xE7B06B); // mute, clip, hot meter segments
inline const juce::Colour rec = rgb(0xE58C98);
inline const juce::Colour recIdle = rec.withMultipliedBrightness(0.48f);

/** An active top bar button's background, in its own colour. */
inline juce::Colour activeFill(juce::Colour inColour)
{
    return inColour.withAlpha(0.14f);
}

/** An active sidebar toggle's background, in its own colour. */
inline juce::Colour toggleFill(juce::Colour inColour)
{
    return inColour.withAlpha(0.18f);
}

inline juce::Colour accentFillActive()
{
    return activeFill(accent); // active transport button
}

inline juce::Colour accentFillToggle()
{
    return toggleFill(accent); // solo, the sidebar "+"
}

inline juce::Colour muteFill()
{
    return toggleFill(warn);
}

inline juce::Colour accentFillButton()
{
    return accent.withAlpha(0.09f); // Drag MIDI out
}

inline juce::Colour accentWashWave()
{
    return accent.withAlpha(0.045f);
}

inline juce::Colour accentWashRoll()
{
    return accent.withAlpha(0.03f);
}

inline juce::Colour accentWashEdge()
{
    return accent.withAlpha(0.14f);
}

// ---- level meters -------------------------------------------------------
// One set of segment colours for every meter, the master's. An instrument's own colour is already
// carried by its chip and its fader, and repeating it here would make "hot" a different hue on
// every row -- which is what the handoff draws, and what it should not.
inline const juce::Colour meterLow = rgb(0x3D9C8C);
inline const juce::Colour meterMid = rgb(0x398DF3);
inline const juce::Colour meterHot = warn;

inline const juce::Colour meterUnlitStrip = rgb(0xD5DFEB);
inline const juce::Colour meterUnlitMaster = rgb(0xD5DFEB);

// ---- waveform -----------------------------------------------------------
inline const juce::Colour wavePlayed = rgb(0x398DF3);
inline const juce::Colour waveUnplayed = rgb(0xA8B9CE);

inline juce::Colour waveCentreLine()
{
    return rgb(0x9BAEC5).withAlpha(0.24f);
}

// ---- piano roll ---------------------------------------------------------
inline const juce::Colour keyWhite = rgb(0xFFFFFF);
inline const juce::Colour keyBlack = rgb(0x6F8198);
inline const juce::Colour keyLabel = textLabel;
inline const juce::Colour laneBlack = rgb(0xE6EDF6);
inline const juce::Colour laneWhite = rgb(0xF4F8FD);

inline juce::Colour noteOnsetEdge()
{
    return rgb(0xA7BED8).withAlpha(0.42f);
}

/** Laid over the part of the roll the transcription has not reached yet. */
inline juce::Colour untranscribedShade()
{
    return bgRoot.withAlpha(0.75f);
}

// ---- fader --------------------------------------------------------------
inline const juce::Colour faderTrack = rgb(0xD5E1EF);
inline const juce::Colour faderTrackTop = rgb(0xCFDCEB); // top bar sliders
inline const juce::Colour faderThumb = rgb(0xFFFFFF);
inline const juce::Colour faderThumbMuted = rgb(0x8EA0B6);
inline const juce::Colour faderFillMuted = rgb(0x91A4BC);
inline const juce::Colour volumeFill = rgb(0x7C91AB);
inline const juce::Colour scrollbarThumb = rgb(0xA5B7CD);

inline juce::Colour mixSliderFill()
{
    return accent.withAlpha(0.8f);
}

// Instrument colours are the one part of the palette that is not here: they are per-instrument
// data, so they live next to the per-instrument names in InstrumentInfo.cpp, and every consumer
// reads them through InstrumentMixer::colourForProgram.

/** Sidebar chip fill / border, derived from the instrument colour. */
inline juce::Colour chipFill(juce::Colour inColour)
{
    return inColour.withAlpha(0.13f);
}

inline juce::Colour chipBorder(juce::Colour inColour)
{
    return inColour.withAlpha(0.25f);
}

/** An unmuted instrument fader's fill, derived from the instrument colour. */
inline juce::Colour faderFill(juce::Colour inColour)
{
    return inColour.withAlpha(0.85f);
}

inline juce::Colour soloRowTint()
{
    return accent.withAlpha(0.05f);
}

// ---- popups -------------------------------------------------------------
inline const juce::Colour popupBg = rgb(0xFFFFFF);
inline const juce::Colour popupBorder = rgb(0xC3D1E2);
inline const juce::Colour popupFooterBg = rgb(0xE9EFF7);
inline const juce::Colour popupRowHover = rgb(0xDDEAF9);
inline const juce::Colour popupTitle = rgb(0x6B7D94);
inline const juce::Colour popupItem = rgb(0x526781); // unticked row label
inline const juce::Colour popupItemTicked = rgb(0x243A55);
inline const juce::Colour checkboxBorder = rgb(0x94A8C0);
inline const juce::Colour checkboxTick = rgb(0xFFFFFF); // drawn on top of the accent fill

inline juce::Colour popupShadow()
{
    return juce::Colours::black.withAlpha(0.16f);
}

// ---- empty states -------------------------------------------------------
inline const juce::Colour ctaBorder = accent;
inline const juce::Colour ctaText = accentText;
inline const juce::Colour dropZoneBorder = rgb(0xB4C9E0);

// No disabled variants here: every disabled control is its enabled self at DISABLED_ALPHA.

inline juce::Colour ctaFill()
{
    return accent.withAlpha(0.11f);
}

inline juce::Colour dropZoneFill()
{
    return accent.withAlpha(0.015f);
}

// ---- transcription progress ---------------------------------------------
inline const juce::Colour progressTrack = faderTrack;
inline const juce::Colour progressFill = accent;
inline const juce::Colour progressText = accentText;

// ---- vertical zoom slider -----------------------------------------------
inline const juce::Colour zoomIcon = textFainter;
inline const juce::Colour zoomTrack = faderTrack;
inline const juce::Colour zoomFill = rgb(0xA2AFBC);
inline const juce::Colour zoomThumb = rgb(0xFFFFFF);
} // namespace nn::colours

namespace nn::metrics
{
constexpr int topBarHeight = 58;
constexpr int toolbarHeight = 46;
constexpr int waveformHeight = 136;
constexpr int rulerHeight = 24;
constexpr int statusBarHeight = 30;

constexpr int sidebarWidth = 292;
constexpr int sidebarHeader = 54;
constexpr int stripHeight = 112;

/** Shared left gutter for the waveform, the ruler and the piano roll -- keeps the time axes aligned. */
constexpr int timelineGutter = 60;

// ---- waveform bars and amplitude axis ------------------------------------
constexpr float waveformBarWidth = 3.0f;
constexpr float waveformBarGap = 1.0f;
constexpr float waveformBarPitch = waveformBarWidth + waveformBarGap;

/** Centre of the 1 px line drawn at waveformHeight / 2, so amplitude 0 lands mid-pixel. */
constexpr float waveformCentreY = waveformHeight * 0.5f + 0.5f;

/**
 * Where amplitude +/-1.0 lands, measured from waveformCentreY. The waveform and the gutter's
 * "+1.0" / "-1.0" labels both derive their y from this, so the axis cannot say one thing and
 * draw another.
 */
constexpr float waveformAmpHalfSpan = 51.0f;

/** One white key's height in the key column at the default vertical zoom. See nn::zoom. */
constexpr int pianoKeyHeight = 13;

// ---- popup menus and the instrument dropdown ----------------------------
constexpr int menuWidth = 380;
constexpr int menuCorner = 16;
constexpr int menuHeaderHeight = 58;
constexpr int menuFooterHeight = 48;
constexpr int menuRowHeight = 48;
constexpr int menuListMaxHeight = 420; // scrolls beyond this
constexpr int menuPadX = 24;
constexpr int menuListPadY = 4;
constexpr int menuAnchorRight = 10; // from the sidebar's right edge
constexpr int menuAnchorTop = 33; // from the top of the sidebar header
constexpr int checkboxSize = 16;
constexpr int checkboxCorner = 3;

// ---- empty-state call to action -----------------------------------------
constexpr int ctaHeight = 38; // Transcribe
constexpr int ctaPadX = 17;
constexpr int ctaGap = 9;
constexpr int loadHeight = 36; // Load audio file
constexpr int loadPadX = 15;
constexpr int loadGap = 8;
constexpr int dropZoneInset = 9;
constexpr int dropZoneCorner = 8;
constexpr int dropHintGap = 9;

// ---- transcription progress, centred over the piano roll ----------------
constexpr int progressBarWidth = 150;
constexpr int progressBarHeight = 4;
constexpr int progressPctWidth = 28;
constexpr int cancelHitSize = 16;
constexpr int cancelGlyphSize = 9;
constexpr int pauseResumeGlyphSize = 10;
constexpr int progressGap = 10;

// ---- vertical zoom slider ------------------------------------------------
constexpr int zoomIconSize = 13;
constexpr int zoomTrackWidth = 84;
constexpr int zoomGap = 10;

constexpr int controlHeight = 34; // top bar pills
constexpr int toolbarButton = 34;
constexpr int transportButtonW = 38;
constexpr int transportButtonH = 34;
constexpr int controlCorner = 8;
constexpr int controlGap = 12; // between the boxes of neighbouring top bar and toolbar controls

constexpr int stripChipSize = 24;
constexpr int stripTextInset = 34; // chip (24) + gap (9) + optical inset (1)

// ---- level meters ----
// The strip meter takes the fader row's own insets, so the two line up under the name.
constexpr int stripMeterSegs = 16;
constexpr int stripMeterGap = 2;
constexpr int stripMeterHeight = 4;
constexpr int stripMeterTopGap = 6; // below the fader row

constexpr int masterMeterSegs = 26;
constexpr int masterMeterGap = 3;
constexpr int masterMeterHeight = 6;
constexpr int masterMeterTopGap = 12; // below the MASTER label row

constexpr float meterCorner = 1.0f;

// The authored (1.0x) editor size. Every extent above is expressed against it, and the editor
// scales the whole UI by an affine transform rather than reflowing, so these never change.
constexpr int editorWidth = 1280;
constexpr int editorHeight = 800;

// How far the editor may be scaled. The upper end is an absolute ceiling; NnEditorConstrainer
// lowers it further to whatever fits the display the window is on.
constexpr double minEditorScale = 0.5; //  640 x  400
constexpr double maxEditorScale = 2.0; // 2560 x 1600
} // namespace nn::metrics

/**
 * The piano roll's vertical zoom, as the slider's normalised position.
 *
 * The design specifies it as a per-semitone lane height, on the assumption of uniform lanes. Our
 * key column is a real keyboard, where a white key spans more than one semitone, so the same
 * numbers are expressed here as a white key's height -- seven white keys to twelve semitones. That
 * reproduces the design's lanes-per-screen at both ends of the slider without touching the piano.
 */
namespace nn::zoom
{
constexpr float rowHeightMin = 6.0f; // per semitone, at norm = 0
constexpr float rowHeightRange = 37.6f;

constexpr int whiteKeysPerOctave = 7;
constexpr int semitonesPerOctave = 12;

/** Never fewer than one octave on screen, which is what caps the zoom in. */
constexpr int minVisibleSemitones = semitonesPerOctave;

inline float keyHeightForRowHeight(float inRowHeight)
{
    return inRowHeight * static_cast<float>(semitonesPerOctave) / static_cast<float>(whiteKeysPerOctave);
}

/** One white key's height at a slider position. 0 is zoomed out. */
inline float keyHeight(float inNorm)
{
    return keyHeightForRowHeight(rowHeightMin + juce::jlimit(0.0f, 1.0f, inNorm) * rowHeightRange);
}

/**
 * The largest zoom that still shows the whole transcribed range, so the view never crops content
 * at rest. Falls back to 0 when the range cannot fit even zoomed all the way out.
 */
inline float fitToContent(int inColumnHeightPx, int inSpanSemitones)
{
    const int span = juce::jmax(minVisibleSemitones, inSpanSemitones);
    const float row_height = static_cast<float>(inColumnHeightPx) / static_cast<float>(span);

    return juce::jlimit(0.0f, 1.0f, (row_height - rowHeightMin) / rowHeightRange);
}
} // namespace nn::zoom

namespace nn
{
/** How a control is being interacted with. Every widget derives its colours from one of these. */
struct ControlState {
    bool isOver = false;
    bool isDown = false;
    bool isOn = false;
    bool isEnabled = true;
};

/** Painted over the whole widget when it is disabled, rather than a second set of colours. */
constexpr float DISABLED_ALPHA = 0.38f;

/** Painted over a whole instrument strip when the instrument is muted. */
constexpr float MUTED_ALPHA = 0.5f;

/**
 * The background a control paints, given its idle and toggled-on fills.
 *
 * Hover lifts an idle control onto the one shared hover surface -- which is what makes a
 * transparent transport button and a filled top-bar pill light up identically. A control that is
 * already on keeps its fill and brightens its foreground instead, so "on" never reads as "hovered".
 */
inline juce::Colour surfaceFor(juce::Colour inIdle, juce::Colour inOn, const ControlState& inState)
{
    juce::Colour surface = inState.isOn ? inOn : inIdle;

    if (!inState.isEnabled)
        return surface;

    if (inState.isDown)
        return (inState.isOn ? surface : nn::colours::bgControlSubtle).darker(0.15f);

    if (inState.isOver && !inState.isOn)
        return nn::colours::bgControlSubtle;

    return surface;
}

/** The icon / label colour matching surfaceFor. */
inline juce::Colour foregroundFor(juce::Colour inIdle, juce::Colour inOn, const ControlState& inState)
{
    const juce::Colour foreground = inState.isOn ? inOn : inIdle;

    if (inState.isEnabled && inState.isOver && inState.isOn)
        return foreground.brighter(0.12f);

    return foreground;
}

inline void drawBottomBorder(juce::Graphics& g, juce::Rectangle<int> inBounds, juce::Colour inColour)
{
    g.setColour(inColour);
    g.fillRect(inBounds.getX(), inBounds.getBottom() - 1, inBounds.getWidth(), 1);
}

inline void drawTopBorder(juce::Graphics& g, juce::Rectangle<int> inBounds, juce::Colour inColour)
{
    g.setColour(inColour);
    g.fillRect(inBounds.getX(), inBounds.getY(), inBounds.getWidth(), 1);
}

inline void drawRightBorder(juce::Graphics& g, juce::Rectangle<int> inBounds, juce::Colour inColour)
{
    g.setColour(inColour);
    g.fillRect(inBounds.getRight() - 1, inBounds.getY(), 1, inBounds.getHeight());
}

/**
 * Draws text with letter-spacing, which JUCE has no API for. The design leans on it for every
 * small-caps label, and without it those labels are visibly tighter than the mockup.
 *
 * @param inTrackingEm Spacing between glyphs as a fraction of the font's em size, as in CSS.
 */
void drawTrackedText(juce::Graphics& g,
                     const juce::String& inText,
                     const juce::Font& inFont,
                     juce::Rectangle<float> inArea,
                     juce::Justification inJustification,
                     float inTrackingEm);

/** @return The width drawTrackedText would occupy, for laying out a row around it. */
float trackedTextWidth(const juce::String& inText, const juce::Font& inFont, float inTrackingEm);

/** @return e.g. "C2" or "F#4" for a MIDI note number, as used by the key labels and the meta lines. */
juce::String midiNoteName(int inMidiNote);

/**
 * The surface every floating panel sits on: menus, tooltips, the update notification.
 *
 * The drop shadow is left to the caller. It falls outside inBounds, so only the caller knows
 * whether the component it is drawing into has the room for it.
 */
void drawPopupSurface(juce::Graphics& g, juce::Rectangle<float> inBounds);

/**
 * The one LookAndFeel, installed as this binary's default by DefaultLookAndFeel below.
 *
 * It exists for the surfaces we do not draw ourselves and cannot reach any other way: popup menus,
 * tooltips, and -- in the standalone -- the title bar's Options button, its menu and the Audio/MIDI
 * settings dialog behind it. JUCE builds all of those itself and resolves them against the default,
 * so without this they arrive in stock grey.
 *
 * Menus draw the same panel the instrument dropdown does -- same surface, same rows, same tick --
 * so that nothing a user opens looks like it came out of a different program than the one they are
 * in. The colour scheme does the same job for the settings dialog, whose widgets are JUCE's own.
 */
class NeuralNoteLookAndFeel : public juce::LookAndFeel_V4
{
public:
    NeuralNoteLookAndFeel();

    juce::Font getPopupMenuFont() override;

    void drawPopupMenuBackgroundWithOptions(juce::Graphics& g,
                                            int inWidth,
                                            int inHeight,
                                            const juce::PopupMenu::Options& inOptions) override;

    void drawPopupMenuItemWithOptions(juce::Graphics& g,
                                      const juce::Rectangle<int>& inArea,
                                      bool inIsHighlighted,
                                      const juce::PopupMenu::Item& inItem,
                                      const juce::PopupMenu::Options& inOptions) override;

    void getIdealPopupMenuItemSizeWithOptions(const juce::String& inText,
                                              bool inIsSeparator,
                                              int inStandardItemHeight,
                                              int& outIdealWidth,
                                              int& outIdealHeight,
                                              const juce::PopupMenu::Options& inOptions) override;

    int getPopupMenuBorderSizeWithOptions(const juce::PopupMenu::Options& inOptions) override;

    juce::Rectangle<int> getTooltipBounds(const juce::String& inTipText,
                                          juce::Point<int> inScreenPos,
                                          juce::Rectangle<int> inParentArea) override;

    void drawTooltip(juce::Graphics& g, const juce::String& inText, int inWidth, int inHeight) override;

    void drawCornerResizer(
        juce::Graphics& g, int inWidth, int inHeight, bool inIsMouseOver, bool inIsMouseDragging) override;
};

/**
 * Holds the look and feel above and installs it for as long as it lives.
 *
 * There is one default per binary rather than per instance, so every open editor shares this one --
 * hence the juce::SharedResourcePointer the processor holds it through. Setting the default does
 * not reach the host or anyone else's plugins: the default lives in a static inside our own binary
 * and is not shared across the dynamic library boundary.
 */
class DefaultLookAndFeel
{
public:
    DefaultLookAndFeel() { juce::LookAndFeel::setDefaultLookAndFeel(&mLookAndFeel); }

    ~DefaultLookAndFeel() { juce::LookAndFeel::setDefaultLookAndFeel(nullptr); }

private:
    NeuralNoteLookAndFeel mLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DefaultLookAndFeel)
};

/**
 * The tick box the instrument dropdown and the popup menus share.
 *
 * @param inArea Where to draw it; the box is centred in this, at its fixed size.
 * @param inAlpha Multiplied into its colours. Graphics::setOpacity would not survive the setColour
 *        calls inside, so the alpha has to come in rather than be set around the call.
 */
void drawCheckbox(juce::Graphics& g, juce::Rectangle<int> inArea, bool inIsTicked, float inAlpha = 1.0f);

/**
 * The two non-ASCII glyphs the UI uses. Built from code points rather than written as literals:
 * juce::String's const char* constructor is documented as ASCII-only, so a UTF-8 literal reaches
 * the screen one mojibake character per byte.
 */
inline juce::String separatorDot()
{
    return juce::String::charToString(0x00b7); // MIDDLE DOT
}

inline juce::String minusSign()
{
    return juce::String::charToString(0x2212); // MINUS SIGN, which lines up with the digits
}
} // namespace nn

#endif // NnLook_h
