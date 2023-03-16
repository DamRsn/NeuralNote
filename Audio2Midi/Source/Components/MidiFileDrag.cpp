//
// Created by Damien Ronssin on 11.03.23.
//

#include "MidiFileDrag.h"

MidiFileDrag::MidiFileDrag(Audio2MidiAudioProcessor& processor)
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
    g.setColour(WHITE_BG);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);

    g.setColour(FONT_BLACK);
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

    auto out_file = mTempDirectory.getChildFile(mFilename + ".mid");

    auto success_midi_file_creation =
        mMidiFileWriter.writeMidiFile(mProcessor.getNoteEventVector(),
                                      out_file,
                                      mProcessor.getPlayheadInfoOnRecordStart(),
                                      mProcessor.getSampleAcquisitionMode());

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

void MidiFileDrag::setFilename(const std::string& inFilename)
{
    mFilename = inFilename;
}
