//
// Created by Damien Ronssin on 28.09.26.
//

#include "ComputeDevices.h"

#include <algorithm>
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

    /**
     * A copy per caller: a shared_future is safe across threads only through separate copies. The
     * static's destructor waits for the listing, so it cannot outlive the plugin binary.
     */
    std::shared_future<std::vector<msl::Device>> listing()
    {
        static const std::shared_future<std::vector<msl::Device>> future =
            std::async(std::launch::async, listDevices).share();
        return future;
    }

    using Support = std::optional<std::vector<msl::DeviceSupport>>;

    Support checkModel(ModelSize inModelSize)
    {
        const std::vector<msl::Device>& devices = get();
        const juce::File model_file = NNFileUtils::getModelFile(inModelSize);

        if (!NNFileUtils::isModelInstalled(inModelSize)) {
            return std::nullopt;
        }

        auto support = msl::Transcriber::checkDevices(NNFileUtils::toPath(model_file));

        if (!support.has_value()) {
            NnLogger::write(NnLogger::Level::Warning,
                            "ComputeDevices: could not check " + model_file.getFileName() + ": "
                                + msl::describe(support.error()));
            return std::nullopt;
        }

        for (std::size_t i = 0; i < devices.size(); ++i) {
            if (!(*support)[i].supported()) {
                NnLogger::write(NnLogger::Level::Warning,
                                "ComputeDevices: " + juce::String(label(devices, i)) + " cannot run "
                                    + model_file.getFileName() + ": unsupported op "
                                    + juce::String((*support)[i].unsupported_op));
            }
        }

        return std::move(*support);
    }

    /** One check per model size, for the life of the process. */
    struct SupportCache {
        std::mutex mutex;
        std::map<ModelSize, std::shared_future<Support>> checks;
    };

    SupportCache& supportCache()
    {
        static SupportCache cache;
        return cache;
    }

    /** @return A copy of inModelSize's check, starting it if there is none or it found nothing. */
    std::shared_future<Support> supportCheck(ModelSize inModelSize)
    {
        SupportCache& cache = supportCache();
        const std::lock_guard<std::mutex> lock(cache.mutex);

        auto it = cache.checks.find(inModelSize);

        // One that found nothing is run again, so a model installed since gets checked. Nothing
        // points into it: the getters return null for it.
        if (it != cache.checks.end() && it->second.wait_for(std::chrono::seconds(0)) == std::future_status::ready
            && !it->second.get().has_value()) {
            cache.checks.erase(it);
            it = cache.checks.end();
        }

        if (it == cache.checks.end()) {
            it = cache.checks.emplace(inModelSize, std::async(std::launch::async, checkModel, inModelSize).share())
                     .first;
        }

        return it->second;
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

void prepareSupport(ModelSize inModelSize)
{
    supportCheck(inModelSize);
}

const std::vector<msl::DeviceSupport>* tryGetSupport(ModelSize inModelSize)
{
    const auto check = supportCheck(inModelSize);

    if (check.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
        return nullptr;
    }

    return pointerTo(check.get());
}

const std::vector<msl::DeviceSupport>* getSupport(ModelSize inModelSize)
{
    return pointerTo(supportCheck(inModelSize).get());
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
