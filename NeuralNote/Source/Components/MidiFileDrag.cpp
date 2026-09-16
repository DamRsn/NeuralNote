//
// Created by Damien Ronssin on 11.03.23.
//

#include "MidiFileDrag.h"

#include "NnFonts.h"
#include "NnIcons.h"
#include "NnLook.h"

MidiFileDrag::MidiFileDrag(NeuralNoteAudioProcessor* inProcessor)
    : NnFlatButton("DragMidiOut")
    , mProcessor(inProcessor)
{
    setIcon(nn::icons::downloadStroked, IconStyle::stroked, 13.0f);
    setLabel("Drag MIDI out", nn::fonts::buttonLabel());
    setPadding(12, 12, 7);

    // The one accent-outlined control in the window: this is the primary way a transcription
    // leaves the plugin.
    setColour(backgroundColourId, nn::colours::accentFillButton());
    setColour(outlineColourId, nn::colours::accent);
    setColour(iconColourId, nn::colours::accentText);
    setColour(textColourId, nn::colours::accentText);

    setTooltip("Drag the transcribed MIDI into your DAW");
}

MidiFileDrag::~MidiFileDrag()
{
    if (mTempDirectory.isDirectory()) {
        mTempDirectory.deleteRecursively();
    }
}

void MidiFileDrag::mouseDown(const juce::MouseEvent& inEvent)
{
    NnFlatButton::mouseDown(inEvent);

    if (!isEnabled()) {
        return;
    }

    if (!NNFileUtils::ensureDirectoryExists(mTempDirectory)) {
        juce::NativeMessageBox::showMessageBoxAsync(
            juce::MessageBoxIconType::NoIcon, "Error", "Temporary directory for midi file failed.");
        return;
    }

    const auto out_file = mTempDirectory.getChildFile(
        NNFileUtils::getMidiExportFileName(mProcessor->getSourceAudioManager()->getDroppedFilename()));
    const double export_bpm = mProcessor->getValueTree().getProperty(NnId::ExportTempoId, 120.0);
    const auto overflow_mode = static_cast<MidiOverflowMode>(static_cast<int>(mProcessor->getValueTree().getProperty(
        NnId::MidiOverflowModeId, static_cast<int>(MidiOverflowMode::ReuseChannels))));

    const bool success = mMidiFileWriter.writeMidiFile(
        mProcessor->getTranscriptionManager()->getNoteEventVector(),
        out_file,
        export_bpm,
        mProcessor->getSourceAudioManager()->getExportStartOffsetSeconds(),
        overflow_mode);

    if (!success) {
        juce::NativeMessageBox::showMessageBoxAsync(
            juce::MessageBoxIconType::NoIcon, "Error", "Could not create the midi file.");
        return;
    }

    juce::DragAndDropContainer::performExternalDragDropOfFiles({out_file.getFullPathName()}, false, this);

    // The drag runs a nested event loop, so the mouse-up that would normally clear the pressed
    // state never reaches this button.
    setState(buttonNormal);
}

void MidiFileDrag::mouseEnter(const juce::MouseEvent& inEvent)
{
    NnFlatButton::mouseEnter(inEvent);
    setMouseCursor(isEnabled() ? juce::MouseCursor::DraggingHandCursor : juce::MouseCursor::NormalCursor);
}

void MidiFileDrag::mouseExit(const juce::MouseEvent& inEvent)
{
    NnFlatButton::mouseExit(inEvent);
    setMouseCursor(juce::MouseCursor::ParentCursor);
}
