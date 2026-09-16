//
// Created by Damien Ronssin on 07.08.26.
//

#include "NnIcons.h"

namespace nn::icons
{
namespace
{
    // Every icon is drawn in this square, so the numbers below can be read straight off the mockup's
    // proportions instead of against a live size.
    constexpr float DESIGN_SIZE = 16.0f;

    /**
 * Maps the whole design square onto inBounds, rather than fitting each path to its own extents.
 * That is the difference between a set of icons that share a weight and a set where the one with
 * the least ink is drawn the largest -- and it lets two paths meant to overlap still overlap.
 */
    Path fitted(Path inPath, Rectangle<float> inBounds)
    {
        const float scale = std::min(inBounds.getWidth(), inBounds.getHeight()) / DESIGN_SIZE;
        const float side = DESIGN_SIZE * scale;

        inPath.applyTransform(AffineTransform::scale(scale).translated(inBounds.getCentreX() - side / 2.0f,
                                                                       inBounds.getCentreY() - side / 2.0f));
        return inPath;
    }
} // namespace

Path skipToStart(Rectangle<float> inBounds)
{
    Path p;
    p.addRoundedRectangle(2.5f, 3.0f, 1.8f, 10.0f, 0.9f);
    p.addTriangle(13.5f, 3.0f, 13.5f, 13.0f, 5.5f, 8.0f);

    return fitted(std::move(p), inBounds);
}

Path play(Rectangle<float> inBounds)
{
    Path p;
    p.addTriangle(4.0f, 2.5f, 4.0f, 13.5f, 13.0f, 8.0f);

    return fitted(std::move(p), inBounds);
}

Path pause(Rectangle<float> inBounds)
{
    Path p;
    p.addRoundedRectangle(4.0f, 2.0f, 3.0f, 12.0f, 1.2f);
    p.addRoundedRectangle(10.0f, 2.0f, 3.0f, 12.0f, 1.2f);

    return fitted(std::move(p), inBounds);
}

Path record(Rectangle<float> inBounds)
{
    Path p;
    p.addEllipse(3.0f, 3.0f, 10.0f, 10.0f);

    return fitted(std::move(p), inBounds);
}

Path loopStroked(Rectangle<float> inBounds)
{
    Path p;
    p.addRoundedRectangle(1.5f, 4.0f, 13.0f, 8.0f, 4.0f);

    return fitted(std::move(p), inBounds);
}

Path loopHead(Rectangle<float> inBounds)
{
    Path p;
    p.addTriangle(6.6f, 5.4f, 6.6f, 10.6f, 10.6f, 8.0f);

    return fitted(std::move(p), inBounds);
}

Path followPlayheadStroked(Rectangle<float> inBounds)
{
    Path p;

    // Left bracket.
    p.startNewSubPath(3.4f, 3.0f);
    p.lineTo(1.6f, 3.0f);
    p.lineTo(1.6f, 13.0f);
    p.lineTo(3.4f, 13.0f);

    // Right bracket.
    p.startNewSubPath(12.6f, 3.0f);
    p.lineTo(14.4f, 3.0f);
    p.lineTo(14.4f, 13.0f);
    p.lineTo(12.6f, 13.0f);

    // The playhead's stem. Its flag is followPlayheadFlag, filled over the top.
    p.startNewSubPath(8.0f, 6.7f);
    p.lineTo(8.0f, 13.2f);

    return fitted(std::move(p), inBounds);
}

Path followPlayheadFlag(Rectangle<float> inBounds)
{
    // Same shape as the marker the timeline draws at the playhead, at icon scale: a flag that
    // tapers to a point where the stem meets it.
    Path p;
    p.startNewSubPath(5.4f, 3.4f);
    p.lineTo(10.6f, 3.4f);
    p.lineTo(10.6f, 5.2f);
    p.lineTo(8.0f, 6.7f);
    p.lineTo(5.4f, 5.2f);
    p.closeSubPath();

    return fitted(std::move(p), inBounds);
}

Path speaker(Rectangle<float> inBounds)
{
    Path p;

    // Cone: the back block plus the flare, as one filled shape.
    p.startNewSubPath(1.5f, 6.0f);
    p.lineTo(4.5f, 6.0f);
    p.lineTo(8.0f, 2.5f);
    p.lineTo(8.0f, 13.5f);
    p.lineTo(4.5f, 10.0f);
    p.lineTo(1.5f, 10.0f);
    p.closeSubPath();

    // One wave, thick enough to survive at 13 px.
    p.addCentredArc(8.6f, 8.0f, 3.0f, 3.0f, 0.0f, 0.6f, 2.55f, true);
    p.addCentredArc(8.6f, 8.0f, 4.1f, 4.1f, 0.0f, 2.55f, 0.6f, false);
    p.closeSubPath();

    return fitted(std::move(p), inBounds);
}

Path speakerMuted(Rectangle<float> inBounds)
{
    Path p;

    p.startNewSubPath(1.0f, 6.0f);
    p.lineTo(4.0f, 6.0f);
    p.lineTo(7.5f, 2.5f);
    p.lineTo(7.5f, 13.5f);
    p.lineTo(4.0f, 10.0f);
    p.lineTo(1.0f, 10.0f);
    p.closeSubPath();

    // The cross, as two filled bars so it reads at the same weight as the cone.
    const float bar = 1.3f;
    Path cross;
    cross.addRectangle(-3.1f, -bar / 2.0f, 6.2f, bar);
    cross.addRectangle(-bar / 2.0f, -3.1f, bar, 6.2f);
    p.addPath(cross, AffineTransform::rotation(MathConstants<float>::pi / 4.0f).translated(11.8f, 8.0f));

    return fitted(std::move(p), inBounds);
}

Path settingsStroked(Rectangle<float> inBounds)
{
    Path p;

    // Three rails with a handle each, offset so they read as faders rather than a hamburger.
    const float rows[] = {4.0f, 8.0f, 12.0f};
    const float handles[] = {10.5f, 5.5f, 9.0f};

    for (int i = 0; i < 3; ++i) {
        p.startNewSubPath(1.5f, rows[i]);
        p.lineTo(14.5f, rows[i]);
        p.startNewSubPath(handles[i], rows[i] - 2.0f);
        p.lineTo(handles[i], rows[i] + 2.0f);
    }

    return fitted(std::move(p), inBounds);
}

Path folderStroked(Rectangle<float> inBounds)
{
    Path p;

    p.startNewSubPath(1.5f, 12.5f);
    p.lineTo(1.5f, 4.0f);
    p.lineTo(6.0f, 4.0f);
    p.lineTo(7.5f, 5.8f);
    p.lineTo(14.5f, 5.8f);
    p.lineTo(14.5f, 12.5f);
    p.closeSubPath();

    return fitted(std::move(p), inBounds);
}

Path downloadStroked(Rectangle<float> inBounds)
{
    Path p;

    p.startNewSubPath(8.0f, 2.0f);
    p.lineTo(8.0f, 10.0f);
    p.startNewSubPath(4.6f, 6.8f);
    p.lineTo(8.0f, 10.2f);
    p.lineTo(11.4f, 6.8f);
    p.startNewSubPath(2.5f, 13.5f);
    p.lineTo(13.5f, 13.5f);

    return fitted(std::move(p), inBounds);
}

Path trashStroked(Rectangle<float> inBounds)
{
    Path p;

    p.startNewSubPath(2.0f, 4.2f);
    p.lineTo(14.0f, 4.2f);

    p.startNewSubPath(6.2f, 4.2f);
    p.lineTo(6.2f, 2.2f);
    p.lineTo(9.8f, 2.2f);
    p.lineTo(9.8f, 4.2f);

    p.startNewSubPath(3.4f, 4.2f);
    p.lineTo(4.2f, 13.8f);
    p.lineTo(11.8f, 13.8f);
    p.lineTo(12.6f, 4.2f);

    return fitted(std::move(p), inBounds);
}

// Built straight from inBounds rather than through the design square: these are the only icons the
// design gives a non-square size (7 x 4), and a uniform fit would shrink them to the shorter side.
Path triangleUp(Rectangle<float> inBounds)
{
    Path p;
    p.addTriangle(inBounds.getX(),
                  inBounds.getBottom(),
                  inBounds.getRight(),
                  inBounds.getBottom(),
                  inBounds.getCentreX(),
                  inBounds.getY());

    return p;
}

Path triangleDown(Rectangle<float> inBounds)
{
    Path p;
    p.addTriangle(inBounds.getX(),
                  inBounds.getY(),
                  inBounds.getRight(),
                  inBounds.getY(),
                  inBounds.getCentreX(),
                  inBounds.getBottom());

    return p;
}

Path plusStroked(Rectangle<float> inBounds)
{
    Path p;
    p.startNewSubPath(8.0f, 2.5f);
    p.lineTo(8.0f, 13.5f);
    p.startNewSubPath(2.5f, 8.0f);
    p.lineTo(13.5f, 8.0f);

    return fitted(std::move(p), inBounds);
}

Path crossStroked(Rectangle<float> inBounds)
{
    Path p;
    p.startNewSubPath(3.2f, 3.2f);
    p.lineTo(12.8f, 12.8f);
    p.startNewSubPath(12.8f, 3.2f);
    p.lineTo(3.2f, 12.8f);

    return fitted(std::move(p), inBounds);
}

Path checkStroked(Rectangle<float> inBounds)
{
    Path p;
    p.startNewSubPath(3.2f, 8.4f);
    p.lineTo(6.3f, 11.5f);
    p.lineTo(12.8f, 4.8f);

    return fitted(std::move(p), inBounds);
}

Path transcribeStroked(Rectangle<float> inBounds)
{
    // Five bars around the centre line, tallest in the middle: a level meter reading as pitches.
    static constexpr float HALF_HEIGHTS[] = {2.4f, 4.4f, 5.4f, 3.4f, 1.4f};

    Path p;

    for (int i = 0; i < 5; i++) {
        const float x = 2.0f + 3.0f * static_cast<float>(i);

        p.startNewSubPath(x, 8.0f - HALF_HEIGHTS[i]);
        p.lineTo(x, 8.0f + HALF_HEIGHTS[i]);
    }

    return fitted(std::move(p), inBounds);
}

Path verticalZoomStroked(Rectangle<float> inBounds)
{
    Path p;
    p.startNewSubPath(8.0f, 2.4f);
    p.lineTo(8.0f, 13.6f);

    p.startNewSubPath(5.4f, 5.0f);
    p.lineTo(8.0f, 2.4f);
    p.lineTo(10.6f, 5.0f);

    p.startNewSubPath(5.4f, 11.0f);
    p.lineTo(8.0f, 13.6f);
    p.lineTo(10.6f, 11.0f);

    return fitted(std::move(p), inBounds);
}
} // namespace nn::icons
