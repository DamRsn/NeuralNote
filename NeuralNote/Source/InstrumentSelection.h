//
// Created by Damien Ronssin on 08.08.26.
//

#ifndef InstrumentSelection_h
#define InstrumentSelection_h

#include <vector>

#include <JuceHeader.h>

#include "muscriptor/note.hpp"

/**
 * Which instruments the next transcription should look for.
 *
 * Empty means the model chooses, which is the default and what every transcription did before this
 * existed. A non-empty selection is not a filter over the result: the library takes it as both a
 * conditioning prefix and a hard mask over what the decoder may emit, so it has to be set before a
 * run starts and changing it means running again.
 *
 * Stored as one comma-separated property on the plugin's state tree rather than as an object, so
 * it saves, restores and defaults along with everything else. Free functions rather than a class
 * for the same reason -- there is no state here that the tree does not already hold.
 *
 * Message thread only.
 */
namespace InstrumentSelection
{
/** @return The selected groups, in enumerator order. Ids the library no longer knows are dropped. */
std::vector<msl::InstrumentGroup> get(const juce::ValueTree& inState);

void set(juce::ValueTree& inState, const std::vector<msl::InstrumentGroup>& inGroups);

bool contains(const juce::ValueTree& inState, msl::InstrumentGroup inGroup);

/** Adds the group if it is absent, removes it if it is present. */
void toggle(juce::ValueTree& inState, msl::InstrumentGroup inGroup);

void clear(juce::ValueTree& inState);

/** @return The program each selected group is decoded as, for the strips the sidebar shows. */
std::vector<int> selectedPrograms(const juce::ValueTree& inState);
} // namespace InstrumentSelection

#endif // InstrumentSelection_h
