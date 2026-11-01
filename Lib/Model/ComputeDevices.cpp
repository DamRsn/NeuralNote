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

    std::optional<Support> checkModel(const juce::File& inModelFile, const ComputeDeviceChoice& inDevice)
    {
        const std::vector<msl::Device>& devices = get();
        const std::optional<std::size_t> index = resolve(devices, inDevice);
        const juce::String target = index.has_value() ? juce::String(label(devices, *index)) : juce::String("Auto");

        const double start_ms = juce::Time::getMillisecondCounterHiRes();
        auto support = msl::Transcriber::checkDevices(NNFileUtils::toPath(inModelFile), index);
        const int elapsed_ms = juce::roundToInt(juce::Time::getMillisecondCounterHiRes() - start_ms);

        if (!support.has_value()) {
            NnLogger::write(NnLogger::Level::Warning,
                            "ComputeDevices: could not check " + inModelFile.getFileName() + " for " + target + ": "
                                + msl::describe(support.error()));
            return std::nullopt;
        }

        juce::StringArray results;
        bool all_usable = true;

        for (std::size_t i = 0; i < devices.size(); ++i) {
            const std::optional<msl::DeviceSupport>& entry = (*support)[i];

            if (!entry.has_value()) {
                continue;
            }

            all_usable = all_usable && entry->supported();
            results.add(juce::String(label(devices, i)) + ": "
                        + (!entry->created()    ? "could not be created: " + juce::String(entry->creation_error)
                           : entry->supported() ? juce::String("supported")
                                                : "cannot run " + juce::String(entry->unsupported_op)));
        }

        NnLogger::write(all_usable ? NnLogger::Level::Info : NnLogger::Level::Warning,
                        "ComputeDevices: checked " + inModelFile.getFileName() + " for " + target + " in "
                            + juce::String(elapsed_ms) + " ms: " + results.joinIntoString(", "));

        return std::move(*support);
    }

    /** A check of one model file for one device choice, valid while the file keeps this modification time. */
    struct CheckRecord {
        ModelSize model_size;
        ComputeDeviceChoice device;
        juce::Time modified;
        Support support;
    };

    struct CheckRecords {
        std::mutex mutex;
        std::vector<CheckRecord> checks; // Oldest first.
    };

    CheckRecords& checkRecords()
    {
        static CheckRecords records;
        return records;
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

std::optional<Support> knownSupport(ModelSize inModelSize)
{
    if (!NNFileUtils::isModelInstalled(inModelSize)) {
        return std::nullopt;
    }

    const juce::Time modified = NNFileUtils::getModelFile(inModelSize).getLastModificationTime();

    CheckRecords& records = checkRecords();
    const std::lock_guard<std::mutex> lock(records.mutex);
    std::optional<Support> found;

    for (auto check = records.checks.rbegin(); check != records.checks.rend(); ++check) {
        if (check->model_size != inModelSize || check->modified != modified) {
            continue;
        }

        if (!found.has_value()) {
            found = check->support;
            continue;
        }

        for (std::size_t i = 0; i < found->size() && i < check->support.size(); ++i) {
            if (!(*found)[i].has_value()) {
                (*found)[i] = check->support[i];
            }
        }
    }

    return found;
}

std::optional<Support> checkSupport(ModelSize inModelSize, const ComputeDeviceChoice& inDevice)
{
    if (!NNFileUtils::isModelInstalled(inModelSize)) {
        return std::nullopt;
    }

    const juce::File model_file = NNFileUtils::getModelFile(inModelSize);
    const juce::Time modified = model_file.getLastModificationTime();
    std::optional<Support> support = checkModel(model_file, inDevice);

    if (support.has_value()) {
        CheckRecords& records = checkRecords();
        const std::lock_guard<std::mutex> lock(records.mutex);

        std::erase_if(records.checks, [&](const CheckRecord& inCheck) {
            return inCheck.model_size == inModelSize && inCheck.device == inDevice;
        });

        records.checks.push_back(
            {.model_size = inModelSize, .device = inDevice, .modified = modified, .support = *support});
    }

    return support;
}

void waitForBackgroundWork()
{
    if (listingStarted) {
        listing().wait();
    }
}

std::size_t autoIndex(const std::vector<msl::Device>& inDevices, const std::optional<Support>& inSupport)
{
    return inSupport.has_value() ? msl::autoDevice(inDevices, *inSupport) : msl::autoDevice(inDevices);
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
