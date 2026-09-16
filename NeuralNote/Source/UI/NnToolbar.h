//
// Created by Damien Ronssin on 07.08.26.
//

#ifndef NnToolbar_h
#define NnToolbar_h

#include <memory>

#include <JuceHeader.h>

#include "MidiFileDrag.h"
#include "MidiFileWriter.h"
#include "NnFlatButton.h"
#include "NumericTextEditor.h"

class NeuralNoteAudioProcessor;

/**
 * The row above the timeline: what is loaded on the left, what can be done with the transcription
 * on the right.
 */
class NnToolbar : public juce::Component
{
public:
    explicit NnToolbar(NeuralNoteAudioProcessor& inProcessor);

    void paint(juce::Graphics& g) override;

    void resized() override;

    /** Re-derives which controls are live. */
    void updateEnablements();

    NnFlatButton& getClearButton() { return mClearButton; }

private:
    void _exportMidiFile();

    /** The bin's right-click menu: everything, or the transcription only. */
    void _showClearMenu();

    void _paintTempoPill(juce::Graphics& g) const;

    NeuralNoteAudioProcessor& mProcessor;

    std::unique_ptr<NumericTextEditor<double>> mTempoEditor;
    NnFlatButton mExportButton {"ExportMidiOut"};
    MidiFileDrag mDragButton;
    NnFlatButton mClearButton {"Clear"};

    juce::Rectangle<int> mTempoPill;

    MidiFileWriter mMidiFileWriter;
    std::shared_ptr<juce::FileChooser> mFileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NnToolbar)
};

#endif // NnToolbar_h
