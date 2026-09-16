//
// Created by Damien Ronssin on 07.08.26.
//

#ifndef NnIcons_h
#define NnIcons_h

#include <JuceHeader.h>

/**
 * The UI's icons, as Paths built in code rather than SVGs in BinaryData.
 *
 * The design tints the same icon differently in almost every state -- idle, hover, active, armed,
 * disabled -- and an SVG bakes its colours in, so each state would need its own file. A Path is
 * filled or stroked in whatever colour the caller is already holding.
 *
 * Every function returns a Path already fitted to inBounds, so callers pass the icon rectangle
 * they laid out and draw the result directly. Stroked icons say so in their name; fill the rest.
 */
namespace nn::icons
{
// ---- transport ----------------------------------------------------------
Path skipToStart(Rectangle<float> inBounds);
Path play(Rectangle<float> inBounds);
Path pause(Rectangle<float> inBounds);
Path record(Rectangle<float> inBounds);

/** The stadium outline. Stroke it, then fill loopHead over the top. */
Path loopStroked(Rectangle<float> inBounds);

/** The play head sitting inside the loop outline. Filled. */
Path loopHead(Rectangle<float> inBounds);

/** Stroke at 1.3 px. The brackets and the stem; fill followPlayheadFlag over the top. */
Path followPlayheadStroked(Rectangle<float> inBounds);

/** The playhead's flag, a miniature of the timeline marker. Filled. */
Path followPlayheadFlag(Rectangle<float> inBounds);

// ---- top bar ------------------------------------------------------------
Path speaker(Rectangle<float> inBounds);

/** Speaker with a cross. The cross is part of the same path, so one fill draws both. */
Path speakerMuted(Rectangle<float> inBounds);

/** Stroke at 1.3 px. Three horizontal rails with offset handles. */
Path settingsStroked(Rectangle<float> inBounds);

// ---- toolbar ------------------------------------------------------------
/** Stroke at 1.3 px. */
Path folderStroked(Rectangle<float> inBounds);

/** Stroke at 1.3 px. Down arrow onto a baseline. */
Path downloadStroked(Rectangle<float> inBounds);

/** Stroke at 1.3 px. */
Path trashStroked(Rectangle<float> inBounds);

// ---- glyphs -------------------------------------------------------------
Path triangleUp(Rectangle<float> inBounds);
Path triangleDown(Rectangle<float> inBounds);

/** Stroke at 1.3 px. */
Path plusStroked(Rectangle<float> inBounds);

/** Stroke at 1.3 px. The cancel cross. */
Path crossStroked(Rectangle<float> inBounds);

/** A tick, drawn on the accent fill of a ticked checkbox. Stroked at 2 px by its caller. */
Path checkStroked(Rectangle<float> inBounds);

/** Stroke at 1.3 px. Five bars of rising then falling height -- audio becoming notes. */
Path transcribeStroked(Rectangle<float> inBounds);

/** Stroke at 1.3 px. A vertical double-headed arrow. */
Path verticalZoomStroked(Rectangle<float> inBounds);

/** Stroke width every "...Stroked" icon above is drawn with, so they all read as one weight. */
inline constexpr float STROKE_WIDTH = 1.3f;
} // namespace nn::icons

#endif // NnIcons_h
