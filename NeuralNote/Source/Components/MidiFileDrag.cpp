//
// Created by Damien Ronssin on 11.03.23.
//

#include "MidiFileDrag.h"

MidiFileDrag::MidiFileDrag(NeuralNoteAudioProcessor& processor)
    : mProcessor(processor)
{
}

MidiFileDrag::~MidiFileDrag()
{
    if (mTempDirectory.isDirectory())
    {
        mTempDirectory.deleteRecursively();
    }
}

void MidiFileDrag::resized()
{
}

void MidiFileDrag::paint(Graphics& g)
{
    g.setColour(WHITE_TRANSPARENT);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);

    g.setColour(BLACK);
    g.setFont(LABEL_FONT);
    g.drawText(
        "DRAG THE MIDI FILE FROM HERE", getLocalBounds(), juce::Justification::centred);
}

void MidiFileDrag::mouseDown(const MouseEvent& event)
{
    if (!mTempDirectory.isDirectory())
    {
        auto result = mTempDirectory.createDirectory();
        if (result.failed())
        {
            NativeMessageBox::showMessageBoxAsync(
                juce::MessageBoxIconType::NoIcon,
                "Error",
                "Temporary directory for midi file failed.");
        }
    }

    std::cout << mTempDirectory.getFullPathName() << std::endl;

    std::string filename = mProcessor.getDroppedFilename();

    if (filename.empty())
        filename = "NNTranscription.mid";
    else
        filename += "_NNTranscription.mid";

    auto out_file = mTempDirectory.getChildFile(filename);

    auto success_midi_file_creation = mMidiFileWriter.writeMidiFile(
        mProcessor.getNoteEventVector(),
        out_file,
        mProcessor.getPlayheadInfoOnRecordStart(),
        mProcessor.getMidiFileTempo(),
        static_cast<PitchBendModes>(
            mProcessor.getCustomParameters()->pitchBendMode.load()));

    if (!success_midi_file_creation)
    {
        NativeMessageBox::showMessageBoxAsync(
            juce::MessageBoxIconType::NoIcon, "Error", "Could not create the midi file.");
    }

    StringArray out_files = {out_file.getFullPathName()};

    DragAndDropContainer::performExternalDragDropOfFiles(out_files, false, this);
}

void MidiFileDrag::mouseEnter(const MouseEvent& event)
{
    setMouseCursor(juce::MouseCursor::DraggingHandCursor);
}

void MidiFileDrag::mouseExit(const MouseEvent& event)
{
    setMouseCursor(juce::MouseCursor::ParentCursor);
}
