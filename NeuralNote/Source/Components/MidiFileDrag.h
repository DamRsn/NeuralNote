//
// Created by Damien Ronssin on 11.03.23.
//

#ifndef MidiFileDrag_h
#define MidiFileDrag_h

#include <JuceHeader.h>

#include "MidiFileWriter.h"
#include "NNFileUtils.h"
#include "NnFlatButton.h"
#include "PluginProcessor.h"

/**
 * The toolbar's "Drag MIDI out": writes the transcription to a temporary .mid and hands it to the
 * system drag, so it can be dropped straight onto a track in the host.
 *
 * A button rather than a bare Component so it gets the shared hover, pressed and disabled
 * treatment; the drag has to start on mouse-down, though, so the click callback is not used.
 */
class MidiFileDrag : public NnFlatButton
{
public:
    explicit MidiFileDrag(NeuralNoteAudioProcessor* inProcessor);

    ~MidiFileDrag() override;

    void mouseDown(const juce::MouseEvent& inEvent) override;

    void mouseEnter(const juce::MouseEvent& inEvent) override;

    void mouseExit(const juce::MouseEvent& inEvent) override;

private:
    NeuralNoteAudioProcessor* mProcessor;

    const juce::File mTempDirectory = NNFileUtils::getMidiDragDirectory();

    MidiFileWriter mMidiFileWriter;
};

#endif // MidiFileDrag_h
