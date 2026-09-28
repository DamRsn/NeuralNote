//
// Created by Damien Ronssin on 29.08.26.
//

#ifndef NnGlobalSettings_h
#define NnGlobalSettings_h

#include <string>

#include "TranscriptionConstants.h"

/**
 * A compute device as the user picked it, by name rather than by index: indices move when drivers
 * or hardware change. Identical devices share a name, so ordinal counts among those.
 */
struct ComputeDeviceChoice {
    // Empty is Auto.
    std::string name;
    int ordinal = 0;

    bool isAuto() const { return name.empty(); }
};

/**
 * The settings that belong to the machine rather than to one plugin instance, kept in
 * <NeuralNote>/global.settings.
 *
 * Getters serve an in-memory copy, refreshed by reload(). Any setter saves every setting, not just
 * the one it changed, so the file lists what the app is using rather than only what the user has
 * touched. That assumes one instance is open at a time.
 *
 * Message thread only -- juce::PropertiesFile is not thread-safe.
 */
namespace NnGlobalSettings
{

/** Re-reads the file. Called when an editor opens, so a new window picks up outside changes. */
void reload();

ModelSize getModelSize();
void setModelSize(ModelSize inModelSize);

/** The scale an editor last applied, already clamped to the display it was on. */
double getEditorScale();
void setEditorScale(double inScale);

bool getTooltipsVisible();
void setTooltipsVisible(bool inVisible);

ComputeDeviceChoice getComputeDevice();
void setComputeDevice(const ComputeDeviceChoice& inChoice);

} // namespace NnGlobalSettings

#endif // NnGlobalSettings_h
