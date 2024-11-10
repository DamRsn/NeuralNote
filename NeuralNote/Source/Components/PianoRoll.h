//
// Created by Damien Ronssin on 10.03.23.
//

#ifndef PianoRoll_h
#define PianoRoll_h

#include <JuceHeader.h>

#include "BasicPitchConstants.h"
#include "Keyboard.h"
#include "PluginProcessor.h"
#include "UIDefines.h"
#include "Playhead.h"

class PianoRoll
    : public Component
    , public ChangeListener
    , ValueTree::Listener
{
public:
    PianoRoll(NeuralNoteAudioProcessor* inProcessor, Keyboard& keyboard, double inBaseNumPixelsPerSecond);

    ~PianoRoll() override;

    void resized() override;

    void paint(Graphics& g) override;

    void changeListenerCallback(ChangeBroadcaster* source) override;

    void mouseDown(const MouseEvent& event) override;

    void setZoomLevel(double inZoomLevel);

private:
    void valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property) override;

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

    void _drawBeatVerticalLines(Graphics& g) const;

    double _beatPosQnToPixel(double inPosQn, double inOffsetBarStart, double inSecondsPerBeat) const;

    const double mBaseNumPixelsPerSecond;
    double mZoomLevel = 1.0;

    ColourGradient mNoteGradient;

    Keyboard& mKeyboard;
    NeuralNoteAudioProcessor* mProcessor;

    Playhead mPlayhead;
};

#endif // PianoRoll_h
