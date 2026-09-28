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
