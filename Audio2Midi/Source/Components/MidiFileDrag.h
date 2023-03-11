//
// Created by Damien Ronssin on 11.03.23.
//

#ifndef MidiFileDrag_h
#define MidiFileDrag_h

#include <JuceHeader.h>

#include "MidiFileWriter.h"
#include "PluginProcessor.h"

class MidiFileDrag : public Component
{
public:
    MidiFileDrag(Audio2MidiAudioProcessor& processor);

    ~MidiFileDrag() override;

    void resized() override;

    void paint(Graphics& g) override;

    void mouseDown(const MouseEvent& event) override;

    void mouseEnter(const MouseEvent& event) override;

    void mouseExit(const MouseEvent& event) override;

    void setFilename(const std::string& inFilename);

private:
    Audio2MidiAudioProcessor& mProcessor;

    std::string mFilename = "TestFilename";

    juce::File mTempDirectory = juce::File::getSpecialLocation(juce::File::tempDirectory);

    MidiFileWriter mMidiFileWriter;
};

#endif // MidiFileDrag_h
