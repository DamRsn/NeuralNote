//
// Created by Damien Ronssin on 11.08.26.
//

#ifndef NNFileUtils_h
#define NNFileUtils_h

#include <filesystem>
#include <optional>

#include <JuceHeader.h>

#include "TranscriptionConstants.h"

/**
 * Every location NeuralNote reads or writes outside what the host hands it. The getters only
 * compute paths; creating one is ensureDirectoryExists's job, called where something is about to
 * be written.
 */
namespace NNFileUtils
{

/** <user application data>/NeuralNote: ~/Library on macOS, %APPDATA% on Windows, ~/.config on Linux. */
juce::File getNeuralNoteDirectory();

/** <NeuralNote>/models, where checkpoints are downloaded to, or put by hand. */
juce::File getModelsDirectory();

/** <NeuralNote>/global.settings, the settings shared by every instance. See NnGlobalSettings. */
juce::File getGlobalSettingsFile();

/** <NeuralNote>/recordings. */
juce::File getRecordingsDirectory();

/** <temp>/neuralnote, for the file a MIDI drag hands the OS and nothing longer. */
juce::File getMidiDragDirectory();

/** Where an "Export MIDI" chooser opens. Not ours: the user's music folder. */
juce::File getDefaultMidiExportDirectory();

/** Creates inDirectory and any missing parent. @return whether it exists afterwards. */
bool ensureDirectoryExists(const juce::File& inDirectory);

/** Prefix of every recording we write, and what isRecordingFile matches on. */
inline constexpr const char* RECORDING_FILENAME_PREFIX = "recorded_audio";

/** The checkpoint for inModelSize, whether or not it is installed. */
juce::File getModelFile(ModelSize inModelSize);

/**
 * Whether the checkpoint is there with the size this build downloads. A file of any other size is
 * unfinished or from another release, and downloading replaces it.
 */
bool isModelInstalled(ModelSize inModelSize);

bool isAnyModelInstalled();

/** inPreferred if it is installed, otherwise another installed size, or nothing if there is none. */
std::optional<ModelSize> getInstalledModelSize(ModelSize inPreferred);

/**
 * Where a download of inModelSize is written until it is verified. Named for the digest it should
 * end up with, so a part started by a build pinned to other weights is never resumed.
 */
juce::File getModelPartFile(ModelSize inModelSize);

/** Opens the models directory in the file manager, creating it first. */
void openModelsDirectory();

/** Whether inFile is a recording we wrote, and so ours to delete. */
bool isRecordingFile(const juce::File& inFile);

/**
 * inFile as a std::filesystem::path. Not getFullPathName().toStdString(): that is UTF-8, and
 * std::filesystem::path decodes a narrow string with the active code page on Windows, mangling any
 * accented directory. The wide representation is native there.
 */
std::filesystem::path toPath(const juce::File& inFile);

/** The name both MIDI exits give the file, e.g. "song_NNTranscription.mid". */
juce::String getMidiExportFileName(const juce::String& inSourceAudioFileName);

} // namespace NNFileUtils

#endif // NNFileUtils_h
