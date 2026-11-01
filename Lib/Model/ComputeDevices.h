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
 * Starts checking inModelSize's checkpoint against the devices on a background thread, if it is
 * installed and has not been checked yet. Thread-safe and cheap after the first call.
 */
void prepareSupport(ModelSize inModelSize);

/**
 * @return Whether each device can run inModelSize's checkpoint, in the order of the device list, or
 *         null while that is still being checked, or when it cannot be (not installed, unreadable).
 *         Never blocks. Thread-safe.
 */
const std::vector<msl::DeviceSupport>* tryGetSupport(ModelSize inModelSize);

/** @return As tryGetSupport, starting the check if needed and waiting for it. Not for the message thread. */
const std::vector<msl::DeviceSupport>* getSupport(ModelSize inModelSize);

/**
 * @return The index of the device Auto runs on: among the devices inSupport marks supported, or
 *         among all of them when inSupport is null.
 */
std::size_t autoIndex(const std::vector<msl::Device>& inDevices, const std::vector<msl::DeviceSupport>* inSupport);

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
