//
// Created by Damien Ronssin on 10.03.23.
//

#ifndef PianoRoll_h
#define PianoRoll_h

#include <JuceHeader.h>

#include "Keyboard.h"
#include "Playhead.h"
#include "PluginProcessor.h"
#include "TranscriptionConstants.h"

class PianoRoll
    : public Component
    , public ChangeListener
{
public:
    PianoRoll(NeuralNoteAudioProcessor* inProcessor, Keyboard& keyboard, double inBaseNumPixelsPerSecond);

    ~PianoRoll() override;

    void resized() override;

    void paint(Graphics& g) override;

    void changeListenerCallback(ChangeBroadcaster* source) override;

    void mouseDown(const MouseEvent& event) override;

    void setZoomLevel(double inZoomLevel);

    /** Hides the playhead while the transcribe button is the thing to look at. */
    void updateEnablements();

private:
    /** Repaints only the sliver the playhead swept, for the same reason AudioRegion does. */
    void _onVBlankCallback();

    float _timeToPixel(float inTime) const;

    float _pixelToTime(float inPixel) const;

    /**
     * Compute rectangle y start and height to draw inNote on piano roll
     * @param inNote Midi note
     * @return Rectangle y start and height
     */
    std::pair<float, float> _getNoteHeightAndWidthPianoRoll(int inNote) const;

    /**
     * Top limit of note on keyboard.
     * @param inNote
     * @return
     */
    float _noteTopY(int inNote) const;

    /** Bottom limit of note on keyboard
     *
     * @param inNote
     * @return
     */
    float _noteBottomY(int inNote) const;

    static bool _isWhiteKey(int inNote);

    float _getNoteWidth(int inNote) const;

    void _drawLanes(Graphics& g) const;

    void _drawNotes(Graphics& g) const;

    /** Shades the stretch a running transcription has not reached yet. Nothing while idle. */
    void _drawTranscriptionFrontier(Graphics& g) const;

    const double mBaseNumPixelsPerSecond;
    double mZoomLevel = 1.0;

    Keyboard& mKeyboard;
    NeuralNoteAudioProcessor* mProcessor;

    Playhead mPlayhead;

    VBlankAttachment mVBlankAttachment;

    int mLastPlayheadX = 0;
};

#endif // PianoRoll_h
