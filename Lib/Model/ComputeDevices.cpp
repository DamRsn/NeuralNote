//
// Created by Damien Ronssin on 28.09.26.
//

#include "ComputeDevices.h"

#include <algorithm>
#include <chrono>
#include <future>

namespace ComputeDevices
{

namespace
{
    /**
     * A copy per caller: a shared_future is safe across threads only through separate copies. The
     * static's destructor waits for the listing, so it cannot outlive the plugin binary.
     */
    std::shared_future<std::vector<msl::Device>> listing()
    {
        static const std::shared_future<std::vector<msl::Device>> future =
            std::async(std::launch::async, [] { return msl::availableDevices(); }).share();
        return future;
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
    const int ordinal = ordinalOf(inDevices, inIndex);
    return ordinal == 0 ? inDevices[inIndex].name : inDevices[inIndex].name + " (" + std::to_string(ordinal + 1) + ")";
}

} // namespace ComputeDevices
