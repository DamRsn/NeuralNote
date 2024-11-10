//
// Created by Damien Ronssin on 11.03.23.
//

#ifndef CombinedAudioMidiRegion_h
#define CombinedAudioMidiRegion_h

#include <JuceHeader.h>

#include "AudioRegion.h"
#include "Keyboard.h"
#include "PianoRoll.h"
#include "PluginProcessor.h"

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

    void resizeAccordingToNumSamplesAvailable();

    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    void setCenterView(bool inShouldCenterView);

    void mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) override;

    void mouseMagnify(const MouseEvent& event, float scaleFactor) override;

    AudioRegion* getAudioRegion();

    PianoRoll* getPianoRoll();

    const double mBaseNumPixelsPerSecond = 100.0;

    const int mAudioRegionHeight = 85;
    const int mHeightBetweenAudioMidi = 23;
    const int mPianoRollY = mAudioRegionHeight + mHeightBetweenAudioMidi;

private:
    void _onVBlankCallback();

    void _centerViewOnPlayhead();

    bool _isFileTypeSupported(const String& filename) const;

    void _setZoomLevel(double inZoomLevel);

    void valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property) override;

    NeuralNoteAudioProcessor* mProcessor;

    juce::Viewport* mViewportPtr = nullptr;
    juce::VBlankAttachment mVBlankAttachment;

    const StringArray mSupportedAudioFileExtensions;

    bool mShouldCenterView = false;

    int mBaseWidth = 0;

    const double mMaxZoomLevel = 5.0;
    const double mMinZoomLevel = 0.1;
    double mZoomLevel = 1.0;

    AudioRegion mAudioRegion;
    PianoRoll mPianoRoll;
};

#endif // CombinedAudioMidiRegion_h
