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
    , public Timer
{
public:
    CombinedAudioMidiRegion(NeuralNoteAudioProcessor& processor, Keyboard& keyboard);

    void setViewportPtr(juce::Viewport* inViewportPtr);

    void resized() override;

    void paint(Graphics& g) override;

    void timerCallback() override;

    bool isInterestedInFileDrag(const StringArray& files) override;

    void filesDropped(const StringArray& files, int x, int y) override;

    void fileDragEnter(const StringArray& files, int x, int y) override;

    void fileDragExit(const StringArray& files) override;

    void setBaseWidth(int inWidth);

    void repaintPianoRoll();

    void resizeAccordingToNumSamplesAvailable();

    void mouseDown(const juce::MouseEvent& e) override;

    const double mNumPixelsPerSecond = 100.0;

    const int mAudioRegionHeight = 85;
    const int mHeightBetweenAudioMidi = 23;
    const int mPianoRollY = mAudioRegionHeight + mHeightBetweenAudioMidi;

private:
    NeuralNoteAudioProcessor& mProcessor;

    juce::Viewport* mViewportPtr = nullptr;

    int mBaseWidth = 0;

    AudioRegion mAudioRegion;
    PianoRoll mPianoRoll;

    std::shared_ptr<juce::FileChooser> mFileChooser;
};

#endif // CombinedAudioMidiRegion_h
