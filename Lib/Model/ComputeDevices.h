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

/**
 * What a check found for each device, in the order of the device list. A device the check did not
 * reach, or could not create, has no entry.
 */
using Support = std::vector<std::optional<msl::DeviceSupport>>;

/**
 * Starts checking inModelSize's checkpoint on a background thread against the devices a load with
 * inDevice would try, if it is installed and that has not been checked for its file yet. Does
 * nothing when inDevice is the CPU, so that choosing the CPU never queries the GPUs. Thread-safe.
 */
void prepareSupport(ModelSize inModelSize, const ComputeDeviceChoice& inDevice);

/**
 * As prepareSupport, then returns everything the finished checks of inModelSize's installed file
 * have found, whatever device they were for, or nullopt when none has finished. Never blocks.
 * Thread-safe.
 */
std::optional<Support> tryGetSupport(ModelSize inModelSize, const ComputeDeviceChoice& inDevice);

/**
 * @return The check of the devices a load of inModelSize with inDevice tries, starting it if needed
 *         and waiting for it, or nullopt when there is none (not installed, unreadable, or inDevice
 *         is the CPU). Not for the message thread.
 */
std::optional<Support> getSupport(ModelSize inModelSize, const ComputeDeviceChoice& inDevice);

/**
 * Waits for the listing and for any support check still running. Call before the plugin binary can
 * unload: they use statics of muscriptor.cpp and ggml that static destruction would otherwise free
 * while they run.
 */
void waitForBackgroundWork();

/**
 * @return The index of the device Auto runs on: passing over the GPUs inSupport marks unsupported,
 *         or the first GPU when inSupport is nullopt.
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
