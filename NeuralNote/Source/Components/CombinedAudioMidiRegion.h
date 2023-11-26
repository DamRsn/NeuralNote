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

    AudioRegion* getAudioRegion();

    PianoRoll* getPianoRoll();

    const double mNumPixelsPerSecond = 100.0;

    const int mAudioRegionHeight = 85;
    const int mHeightBetweenAudioMidi = 23;
    const int mPianoRollY = mAudioRegionHeight + mHeightBetweenAudioMidi;

private:
    void _onVBlankCallback();

    void _centerViewOnPlayhead();

    NeuralNoteAudioProcessor* mProcessor;

    juce::Viewport* mViewportPtr = nullptr;
    juce::VBlankAttachment mVBlankAttachment;

    bool mShouldCenterView = false;

    int mBaseWidth = 0;

    const float mMaxZoomLevel = 5.f;
    const float mMinZoomLevel = 0.1f;
    float mZoomLevel = 1.f;

    AudioRegion mAudioRegion;
    PianoRoll mPianoRoll;
};

#endif // CombinedAudioMidiRegion_h
