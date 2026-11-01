//
// Created by Damien Ronssin on 28.09.26.
//

#ifndef ComputeDevices_h
#define ComputeDevices_h

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "muscriptor/device.hpp"

#include "NnGlobalSettings.h"
#include "TranscriptionConstants.h"

/**
 * The devices muscriptor.cpp can run on, listed once per process on a background thread: the first
 * listing initialises the GPU backends, which can take seconds. The list never changes afterwards,
 * as the library enumerates GPUs once per process.
 */
namespace ComputeDevices
{

/** Starts the listing if it has not started yet. Thread-safe and cheap after the first call. */
void prepare();

/** @return The devices, or null while they are still being listed. Never blocks. Thread-safe. */
const std::vector<msl::Device>* tryGet();

/** @return The devices, waiting for the listing if needed. Not for the message thread. */
const std::vector<msl::Device>& get();

/** What a check found for each device, in the order of the device list. A device it did not reach has no entry. */
using Support = std::vector<std::optional<msl::DeviceSupport>>;

/**
 * @return What the recorded checks of inModelSize's installed file have found, the newest first
 *         for each device, whatever device choice they were for, or nullopt when there are none.
 *         Never blocks and touches no device. Thread-safe.
 */
std::optional<Support> knownSupport(ModelSize inModelSize);

/**
 * Checks the devices a load of inModelSize with inDevice tries, and records the result for
 * knownSupport. Call after that load, so the check touches only devices the load has already
 * created or failed to create. Blocks; not for the message thread. Thread-safe.
 *
 * @return The result, or nullopt when the model is not installed or could not be checked.
 */
std::optional<Support> checkSupport(ModelSize inModelSize, const ComputeDeviceChoice& inDevice);

/**
 * Waits for the listing if it is still running. Call before the plugin binary can unload: it uses
 * statics of muscriptor.cpp and ggml that static destruction would otherwise free while it runs.
 */
void waitForBackgroundWork();

/**
 * @return The index of the device Auto runs on as far as inSupport knows, as msl::autoDevice: the
 *         first GPU in Auto order when nothing is known.
 */
std::size_t autoIndex(const std::vector<msl::Device>& inDevices, const std::optional<Support>& inSupport);

/**
 * @return The index inChoice names in inDevices, or nullopt for Auto, which is also what a device
 *         that is no longer there resolves to.
 */
std::optional<std::size_t> resolve(const std::vector<msl::Device>& inDevices, const ComputeDeviceChoice& inChoice);

/** @return The choice that names inDevices[inIndex]. */
ComputeDeviceChoice choiceFor(const std::vector<msl::Device>& inDevices, std::size_t inIndex);

/** @return The name to show for inDevices[inIndex], numbered when identical devices share it. */
std::string label(const std::vector<msl::Device>& inDevices, std::size_t inIndex);

} // namespace ComputeDevices

#endif // ComputeDevices_h
