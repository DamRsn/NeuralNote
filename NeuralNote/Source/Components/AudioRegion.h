//
// Created by Damien Ronssin on 09.03.23.
//

#ifndef AudioRegion_h
#define AudioRegion_h

#include <JuceHeader.h>

#include "AudioUtils.h"
#include "NnFlatButton.h"
#include "Playhead.h"
#include "PluginProcessor.h"

class CombinedAudioMidiRegion;

class AudioRegion : public Component
{
public:
    AudioRegion(NeuralNoteAudioProcessor* processor, double inBaseNumPixelsPerSecond);

    void resized() override;

    void paint(Graphics& g) override;

    void setIsFileOver(bool inIsFileOver);

    void mouseDown(const juce::MouseEvent& e) override;

    void setZoomLevel(double inZoomLevel);

    /**
     * Draws each bar mirrored about the centre line, at max(|min|, |max|), instead of spanning the
     * slice's actual min to max. On by default.
     */
    void setSymmetricBars(bool inSymmetric);

    /** Shows the load button only while there is nothing to draw a waveform from. */
    void updateEnablements();

private:
    /**
     * One bar per WaveformBars pitch, over the exposed part of the region only. The vertical scale
     * is absolute -- amplitude 1.0 is the "+1.0" the gutter prints, not the loudest sample -- so a
     * quiet recording draws as a quiet waveform.
     */
    void _paintWaveform(Graphics& g) const;

    /** The dashed panel and its hint, drawn behind the load button while the region is empty. */
    void _paintDropZone(Graphics& g) const;

    void _openFileChooser();

    /**
     * Repaints only the sliver the playhead has swept since the last frame. The played and unplayed
     * halves of the waveform are two clipped passes over the thumbnail, so the drawing does depend
     * on the playhead -- but redrawing the whole waveform 60 times a second to move a 1 px line
     * would not be worth what it buys.
     */
    void _onVBlankCallback();

    float _pixelToTime(float inPixel) const;

    NeuralNoteAudioProcessor* mProcessor;

    Playhead mPlayhead;

    VBlankAttachment mVBlankAttachment;

    // The only way in to the file chooser.
    NnFlatButton mLoadButton {"LoadAudio"};

    std::shared_ptr<juce::FileChooser> mFileChooser;

    const double mBaseNumPixelsPerSecond;
    double mZoomLevel = 1.0;

    int mLastPlayheadX = 0;
    bool mIsFileOver = false;
    bool mSymmetricBars = true;
};

#endif // AudioRegion_h
