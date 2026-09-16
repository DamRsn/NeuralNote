//
// Created by Damien Ronssin on 11.03.23.
//

#ifndef CombinedAudioMidiRegion_h
#define CombinedAudioMidiRegion_h

#include <JuceHeader.h>

#include "AudioRegion.h"
#include "Keyboard.h"
#include "NnLook.h"
#include "PianoRoll.h"
#include "PluginProcessor.h"
#include "TimeRuler.h"

class CombinedAudioMidiRegion
    : public Component
    , public FileDragAndDropTarget
    , public ChangeListener
    , public ValueTree::Listener
{
public:
    CombinedAudioMidiRegion(NeuralNoteAudioProcessor* processor, Keyboard& keyboard);

    ~CombinedAudioMidiRegion() override;

    void setViewportPtr(juce::Viewport* inViewportPtr);

    void resized() override;

    void paint(Graphics& g) override;

    bool isInterestedInFileDrag(const StringArray& files) override;

    void filesDropped(const StringArray& files, int x, int y) override;

    void fileDragEnter(const StringArray& files, int x, int y) override;

    void fileDragExit(const StringArray& files) override;

    void setBaseWidth(int inWidth);

    void repaintPianoRoll();

    /** Re-derives what the regions show from the processor's state. */
    void updateEnablements();

    void resizeAccordingToNumSamplesAvailable();

    /**
     * Resizes, and pulls the zoom back inside what the current audio length allows. Use this rather
     * than resizeAccordingToNumSamplesAvailable() wherever the amount of audio may have changed:
     * the lower zoom bound is derived from it.
     */
    void refreshForAudioLength();

    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    void setCenterView(bool inShouldCenterView);

    void mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) override;

    void mouseMagnify(const MouseEvent& event, float scaleFactor) override;

    AudioRegion* getAudioRegion();

    PianoRoll* getPianoRoll();

    const double mBaseNumPixelsPerSecond = 100.0;

    // The waveform, the ruler and the piano roll are stacked inside this one scrolling region, so
    // their time axes cannot drift apart -- which is the whole reason the ruler lives here.
    static constexpr int mAudioRegionHeight = nn::metrics::waveformHeight;
    static constexpr int mRulerHeight = nn::metrics::rulerHeight;
    static constexpr int mPianoRollY = mAudioRegionHeight + mRulerHeight;

private:
    void _onVBlankCallback();

    void _centerViewOnPlayhead();

    bool _isFileTypeSupported(const String& filename) const;

    void _setZoomLevel(double inZoomLevel);

    /**
     * The furthest out the view may go: the zoom at which the audio exactly fills the viewport.
     * Zooming past it would only add empty timeline to the right of the take.
     */
    double _minZoomLevel() const;

    void valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property) override;

    NeuralNoteAudioProcessor* mProcessor;

    // The pitch axis, shared with the key column outside this region: a wheel gesture that lands on
    // the roll has to move both.
    Keyboard& mKeyboard;

    juce::Viewport* mViewportPtr = nullptr;
    juce::VBlankAttachment mVBlankAttachment;

    const StringArray mSupportedAudioFileExtensions;

    bool mShouldCenterView = false;

    int mBaseWidth = 0;

    const double mMaxZoomLevel = 5.0;
    const double mMinZoomLevel = 0.1;
    double mZoomLevel = 1.0;

    AudioRegion mAudioRegion;
    TimeRuler mTimeRuler;
    PianoRoll mPianoRoll;
};

#endif // CombinedAudioMidiRegion_h
