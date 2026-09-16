//
// Created by Damien Ronssin on 08.08.26.
//

#include "InstrumentSelection.h"

#include <algorithm>

#include "NnId.h"

namespace
{
bool isNamedGroup(int inGroupId)
{
    const std::span<const msl::InstrumentGroup> all = msl::allInstrumentGroups();

    return std::find(all.begin(), all.end(), static_cast<msl::InstrumentGroup>(inGroupId)) != all.end();
}
} // namespace

std::vector<msl::InstrumentGroup> InstrumentSelection::get(const juce::ValueTree& inState)
{
    const juce::String stored = inState.getProperty(NnId::SelectedInstrumentGroupsId, juce::String());

    if (stored.isEmpty()) {
        return {};
    }

    juce::StringArray tokens;
    tokens.addTokens(stored, ",", "");

    std::vector<msl::InstrumentGroup> groups;

    for (const juce::String& token: tokens) {
        const juce::String trimmed = token.trim();

        // Anything the library does not name is dropped rather than trusted: the ids come from a
        // saved session, which may have been written by a build with a different group list.
        if (trimmed.containsOnly("0123456789") && trimmed.isNotEmpty() && isNamedGroup(trimmed.getIntValue())) {
            const auto group = static_cast<msl::InstrumentGroup>(trimmed.getIntValue());

            if (std::find(groups.begin(), groups.end(), group) == groups.end()) {
                groups.push_back(group);
            }
        }
    }

    // Enumerator order, so the sidebar and the menu agree however the property was written.
    std::sort(groups.begin(), groups.end());

    return groups;
}

void InstrumentSelection::set(juce::ValueTree& inState, const std::vector<msl::InstrumentGroup>& inGroups)
{
    juce::StringArray tokens;

    for (const msl::InstrumentGroup group: inGroups) {
        tokens.add(juce::String(static_cast<int>(group)));
    }

    inState.setProperty(NnId::SelectedInstrumentGroupsId, tokens.joinIntoString(","), nullptr);
}

bool InstrumentSelection::contains(const juce::ValueTree& inState, msl::InstrumentGroup inGroup)
{
    const std::vector<msl::InstrumentGroup> groups = get(inState);

    return std::find(groups.begin(), groups.end(), inGroup) != groups.end();
}

void InstrumentSelection::toggle(juce::ValueTree& inState, msl::InstrumentGroup inGroup)
{
    std::vector<msl::InstrumentGroup> groups = get(inState);
    const auto it = std::find(groups.begin(), groups.end(), inGroup);

    if (it != groups.end()) {
        groups.erase(it);
    } else {
        groups.push_back(inGroup);
        std::sort(groups.begin(), groups.end());
    }

    set(inState, groups);
}

void InstrumentSelection::clear(juce::ValueTree& inState)
{
    inState.setProperty(NnId::SelectedInstrumentGroupsId, juce::String(), nullptr);
}

std::vector<int> InstrumentSelection::selectedPrograms(const juce::ValueTree& inState)
{
    std::vector<int> programs;

    for (const msl::InstrumentGroup group: get(inState)) {
        programs.push_back(msl::programFor(group));
    }

    return programs;
}
