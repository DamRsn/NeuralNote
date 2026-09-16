//
// Created by Damien Ronssin on 11.08.26.
//

#include "NNFileUtils.h"

#include <algorithm>

#include "ModelManifest.h"

namespace NNFileUtils
{

juce::File getNeuralNoteDirectory()
{
    return juce::File::getSpecialLocation(juce::File::SpecialLocationType::userApplicationDataDirectory)
        .getChildFile("NeuralNote");
}

juce::File getModelsDirectory()
{
    return getNeuralNoteDirectory().getChildFile("models");
}

juce::File getGlobalSettingsFile()
{
    return getNeuralNoteDirectory().getChildFile("global.settings");
}

juce::File getRecordingsDirectory()
{
    return getNeuralNoteDirectory().getChildFile("recordings");
}

juce::File getMidiDragDirectory()
{
    return juce::File::getSpecialLocation(juce::File::SpecialLocationType::tempDirectory).getChildFile("neuralnote");
}

juce::File getDefaultMidiExportDirectory()
{
    return juce::File::getSpecialLocation(juce::File::SpecialLocationType::userMusicDirectory);
}

bool ensureDirectoryExists(const juce::File& inDirectory)
{
    return inDirectory.isDirectory() || inDirectory.createDirectory().wasOk();
}

juce::File getModelFile(ModelSize inModelSize)
{
    return getModelsDirectory().getChildFile(getModelManifestEntry(inModelSize).fileName);
}

bool isModelInstalled(ModelSize inModelSize)
{
    // getSize is 0 for a file that does not exist, and no checkpoint is empty.
    return getModelFile(inModelSize).getSize() == getModelManifestEntry(inModelSize).numBytes;
}

bool isAnyModelInstalled()
{
    return std::any_of(ALL_MODEL_SIZES.begin(), ALL_MODEL_SIZES.end(), isModelInstalled);
}

std::optional<ModelSize> getInstalledModelSize(ModelSize inPreferred)
{
    if (isModelInstalled(inPreferred)) {
        return inPreferred;
    }

    if (isModelInstalled(DEFAULT_MODEL_SIZE)) {
        return DEFAULT_MODEL_SIZE;
    }

    for (const ModelSize size: ALL_MODEL_SIZES) {
        if (isModelInstalled(size)) {
            return size;
        }
    }

    return std::nullopt;
}

juce::File getModelPartFile(ModelSize inModelSize)
{
    const ModelManifestEntry& entry = getModelManifestEntry(inModelSize);

    return getModelsDirectory().getChildFile(juce::String(entry.fileName) + "."
                                             + juce::String(entry.sha256).substring(0, 8) + ".part");
}

void openModelsDirectory()
{
    const juce::File directory = getModelsDirectory();

    if (ensureDirectoryExists(directory)) {
        directory.startAsProcess();
    }
}

bool isRecordingFile(const juce::File& inFile)
{
    // The name is checked as well as the parent, so an audio file the user dropped from that
    // directory is never mistaken for one of ours.
    return inFile.getParentDirectory() == getRecordingsDirectory()
           && inFile.getFileName().startsWith(RECORDING_FILENAME_PREFIX);
}

std::filesystem::path toPath(const juce::File& inFile)
{
#if JUCE_WINDOWS
    return {inFile.getFullPathName().toWideCharPointer()};
#else
    return {inFile.getFullPathName().toStdString()};
#endif
}

juce::String getMidiExportFileName(const juce::String& inSourceAudioFileName)
{
    if (inSourceAudioFileName.isEmpty()) {
        return "NNTranscription.mid";
    }

    return inSourceAudioFileName + "_NNTranscription.mid";
}

} // namespace NNFileUtils
