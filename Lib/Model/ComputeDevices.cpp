//
// Created by Damien Ronssin on 28.09.26.
//

#include "ComputeDevices.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <future>
#include <map>
#include <mutex>

#include "muscriptor/muscriptor.hpp"

#include "NNFileUtils.h"
#include "NnLogger.h"

namespace ComputeDevices
{

namespace
{
    std::vector<msl::Device> listDevices()
    {
        NnLogger::write(NnLogger::Level::Info, "ComputeDevices: listing devices");

        std::vector<msl::Device> devices = msl::availableDevices();

        for (const msl::Device& device: devices) {
            NnLogger::write(NnLogger::Level::Info,
                            "ComputeDevices: " + juce::String(device.name) + " (" + juce::String(device.backend)
                                + (device.integrated ? ", integrated" : "") + "), "
                                + (device.memory_total > 0 ? juce::File::descriptionOfSizeInBytes(
                                                                 static_cast<juce::int64>(device.memory_total))
                                                           : juce::String("memory unknown")));
        }

        return devices;
    }

    std::atomic<bool> listingStarted {false};

    /** A copy per caller: a shared_future is safe across threads only through separate copies. */
    std::shared_future<std::vector<msl::Device>> listing()
    {
        static const std::shared_future<std::vector<msl::Device>> future =
            std::async(std::launch::async, listDevices).share();
        listingStarted = true;
        return future;
    }

    using Support = std::optional<std::vector<msl::DeviceSupport>>;

    Support checkModel(const juce::File& inModelFile)
    {
        const std::vector<msl::Device>& devices = get();
        auto support = msl::Transcriber::checkDevices(NNFileUtils::toPath(inModelFile));

        if (!support.has_value()) {
            NnLogger::write(NnLogger::Level::Warning,
                            "ComputeDevices: could not check " + inModelFile.getFileName() + ": "
                                + msl::describe(support.error()));
            return std::nullopt;
        }

        for (std::size_t i = 0; i < devices.size(); ++i) {
            if (!(*support)[i].supported()) {
                NnLogger::write(NnLogger::Level::Warning,
                                "ComputeDevices: " + juce::String(label(devices, i)) + " cannot run "
                                    + inModelFile.getFileName() + ": unsupported op "
                                    + juce::String((*support)[i].unsupported_op));
            }
        }

        return std::move(*support);
    }

    /** A check of one model file, valid while the file keeps this modification time. */
    struct CachedCheck {
        juce::Time modified;
        std::shared_future<Support> result;
    };

    struct SupportCache {
        std::mutex mutex;
        std::map<ModelSize, CachedCheck> checks;
    };

    SupportCache& supportCache()
    {
        static SupportCache cache;
        return cache;
    }

    /**
     * @return A copy of the check of inModelSize's installed file, started if that file has not been
     *         checked, or nullopt when there is nothing to check: the model is not installed, or
     *         inDevice is the CPU, which never needs the GPUs queried.
     */
    std::optional<std::shared_future<Support>> supportCheck(ModelSize inModelSize, const ComputeDeviceChoice& inDevice)
    {
        if (inDevice.name == "CPU" || !NNFileUtils::isModelInstalled(inModelSize)) {
            return std::nullopt;
        }

        const juce::File model_file = NNFileUtils::getModelFile(inModelSize);
        const juce::Time modified = model_file.getLastModificationTime();

        SupportCache& cache = supportCache();
        const std::lock_guard<std::mutex> lock(cache.mutex);
        CachedCheck& check = cache.checks[inModelSize];

        if (!check.result.valid() || check.modified != modified) {
            check = {.modified = modified, .result = std::async(std::launch::async, checkModel, model_file).share()};
        }

        return check.result;
    }

    const std::vector<msl::DeviceSupport>* pointerTo(const Support& inSupport)
    {
        return inSupport.has_value() ? &*inSupport : nullptr;
    }

    /** @return How many devices before inIndex share its name. */
    int ordinalOf(const std::vector<msl::Device>& inDevices, std::size_t inIndex)
    {
        const std::string& name = inDevices[inIndex].name;
        return static_cast<int>(std::count_if(inDevices.begin(),
                                              inDevices.begin() + static_cast<std::ptrdiff_t>(inIndex),
                                              [&name](const msl::Device& inDevice) { return inDevice.name == name; }));
    }
} // namespace

void prepare()
{
    listing();
}

const std::vector<msl::Device>* tryGet()
{
    const auto future = listing();

    if (future.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
        return nullptr;
    }

    return &future.get();
}

const std::vector<msl::Device>& get()
{
    return listing().get();
}

void prepareSupport(ModelSize inModelSize, const ComputeDeviceChoice& inDevice)
{
    supportCheck(inModelSize, inDevice);
}

const std::vector<msl::DeviceSupport>* tryGetSupport(ModelSize inModelSize, const ComputeDeviceChoice& inDevice)
{
    const auto check = supportCheck(inModelSize, inDevice);

    if (!check.has_value() || check->wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
        return nullptr;
    }

    return pointerTo(check->get());
}

const std::vector<msl::DeviceSupport>* getSupport(ModelSize inModelSize, const ComputeDeviceChoice& inDevice)
{
    const auto check = supportCheck(inModelSize, inDevice);
    return check.has_value() ? pointerTo(check->get()) : nullptr;
}

void waitForBackgroundWork()
{
    {
        SupportCache& cache = supportCache();
        const std::lock_guard<std::mutex> lock(cache.mutex);

        for (const auto& [model_size, check]: cache.checks) {
            check.result.wait();
        }
    }

    if (listingStarted) {
        listing().wait();
    }
}

std::size_t autoIndex(const std::vector<msl::Device>& inDevices, const std::vector<msl::DeviceSupport>* inSupport)
{
    return inSupport != nullptr ? msl::autoDevice(inDevices, *inSupport) : msl::autoDevice(inDevices);
}

std::optional<std::size_t> resolve(const std::vector<msl::Device>& inDevices, const ComputeDeviceChoice& inChoice)
{
    if (inChoice.isAuto()) {
        return std::nullopt;
    }

    for (std::size_t i = 0; i < inDevices.size(); ++i) {
        if (choiceFor(inDevices, i) == inChoice) {
            return i;
        }
    }

    return std::nullopt;
}

ComputeDeviceChoice choiceFor(const std::vector<msl::Device>& inDevices, std::size_t inIndex)
{
    return {.name = inDevices[inIndex].name, .ordinal = ordinalOf(inDevices, inIndex)};
}

std::string label(const std::vector<msl::Device>& inDevices, std::size_t inIndex)
{
    const msl::Device& device = inDevices[inIndex];

    // Metal names the GPU after the chip, which is also what Apple calls the CPU.
    const std::string name = device.backend == "Metal" ? device.name + " GPU" : device.name;

    const int ordinal = ordinalOf(inDevices, inIndex);
    return ordinal == 0 ? name : name + " (" + std::to_string(ordinal + 1) + ")";
}

} // namespace ComputeDevices
