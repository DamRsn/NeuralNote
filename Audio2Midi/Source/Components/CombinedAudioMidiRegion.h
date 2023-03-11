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
    CombinedAudioMidiRegion(Audio2MidiAudioProcessor& processor, Keyboard& keyboard);

    void paint(Graphics& g) override;

    void resized() override;

    void timerCallback() override;

    bool isInterestedInFileDrag(const StringArray& files) override;

    void filesDropped(const StringArray& files, int x, int y) override;

    void fileDragEnter(const StringArray& files, int x, int y) override;

    void fileDragExit(const StringArray& files) override;

    void setBaseWidth(int inWidth);

    const double mNumPixelPerSeconds = 60.0;

    const int mAudioRegionHeight = 100;
    const int mHeightBetweenAudioMidi = 20;
    const int mPianoRollY = mAudioRegionHeight + mHeightBetweenAudioMidi;

private:
    void _resizeAccordingToNumSamplesAvailable();

    Audio2MidiAudioProcessor& mProcessor;

    int mBaseWidth = 0;

    AudioRegion mAudioRegion;
    PianoRoll mPianoRoll;
};

#endif // CombinedAudioMidiRegion_h
