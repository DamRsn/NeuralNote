//
// Created by Damien Ronssin on 07.08.26.
//

#include "InstrumentMixer.h"

#include <algorithm>

#include "GainConstants.h"
#include "InstrumentInfo.h"
#include "PluginProcessor.h"

InstrumentMixer::InstrumentMixer(NeuralNoteAudioProcessor* inProcessor)
    : mProcessor(inProcessor)
{
    // Listening to the whole state tree, not to the mixer child: a state reload replaces that child
    // wholesale, and a listener attached to it would be left watching a detached copy.
    mProcessor->addListenerToStateValueTree(this);
}

InstrumentMixer::~InstrumentMixer()
{
    mProcessor->removeListenerFromStateValueTree(this);
}

void InstrumentMixer::rebuildFromNotes(const std::vector<NoteEvent>& inNotes)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    mTallies = {};

    for (const NoteEvent& note: inNotes) {
        if (note.program < 0 || note.program >= NUM_INSTRUMENT_IDS) {
            jassertfalse;
            continue;
        }

        Tally& tally = mTallies[static_cast<std::size_t>(note.program)];
        tally.noteCount++;
        tally.lowestPitch = std::min(tally.lowestPitch, note.pitch);
        tally.highestPitch = std::max(tally.highestPitch, note.pitch);
    }

    _rebuildEntries();
}

void InstrumentMixer::setSelectedPrograms(std::vector<int> inPrograms)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (inPrograms == mSelectedPrograms) {
        return;
    }

    mSelectedPrograms = std::move(inPrograms);
    _rebuildEntries();
}

void InstrumentMixer::_rebuildEntries()
{
    std::vector<InstrumentEntry> entries;
    mTotalNoteCount = 0;
    mLowestPitch = 127;
    mHighestPitch = 0;

    // Ascending program order, which puts drums (128) last and is stable as the transcription
    // streams in. Ordering by first appearance would reshuffle the sidebar each time a chunk
    // introduced an instrument with a lower program.
    for (int program = 0; program < NUM_INSTRUMENT_IDS; program++) {
        const Tally& tally = mTallies[static_cast<std::size_t>(program)];
        const bool is_selected =
            std::find(mSelectedPrograms.begin(), mSelectedPrograms.end(), program) != mSelectedPrograms.end();

        if (tally.noteCount == 0 && !is_selected) {
            continue;
        }

        const InstrumentDisplay display = instrumentDisplayFor(program);

        entries.push_back({program,
                           display.name,
                           display.abbreviation,
                           instrumentColourFor(program),
                           tally.noteCount,
                           tally.noteCount > 0 ? tally.lowestPitch : 0,
                           tally.noteCount > 0 ? tally.highestPitch : 0});

        mTotalNoteCount += tally.noteCount;

        if (tally.noteCount > 0) {
            mLowestPitch = std::min(mLowestPitch, tally.lowestPitch);
            mHighestPitch = std::max(mHighestPitch, tally.highestPitch);
        }
    }

    if (mTotalNoteCount == 0) {
        mLowestPitch = 0;
        mHighestPitch = 0;
    }

    if (entries == mEntries) {
        return;
    }

    mEntries = std::move(entries);

    // An instrument that has just appeared may already have a stored fader, from a session reload.
    // This is where that reaches the synth -- the synth only learns of the instrument moments
    // before, in _ensureSynthInstruments.
    _pushAllToSynth();
    sendChangeMessage();
}

void InstrumentMixer::clear()
{
    mTallies = {};

    // Not mSelectedPrograms: the selection outlives the transcription it was used for, so the
    // strips it puts on screen stay after a clear, ready for the next run.
    _rebuildEntries();
}

void InstrumentMixer::resetStoredSettings()
{
    juce::ValueTree& state = mProcessor->getValueTree();

    // Removed rather than emptied node by node: an instrument the new transcription does not
    // contain would otherwise keep a node around for the next one to inherit. The listener below
    // is what pushes the neutral values to the synth and refreshes the strips.
    state.removeChild(state.getChildWithName(NnId::InstrumentMixerId), nullptr);
}

const InstrumentEntry* InstrumentMixer::findEntry(int inProgram) const
{
    const auto it = std::find_if(
        mEntries.begin(), mEntries.end(), [inProgram](const InstrumentEntry& e) { return e.program == inProgram; });

    return it == mEntries.end() ? nullptr : &(*it);
}

juce::Colour InstrumentMixer::colourForProgram(int inProgram) const
{
    if (const InstrumentEntry* entry = findEntry(inProgram)) {
        return entry->colour;
    }

    // Reachable only between a note arriving and the rebuild that follows it. The colour is the
    // instrument's own either way, so that window is invisible rather than a frame of grey.
    return instrumentColourFor(inProgram);
}

float InstrumentMixer::getGainDb(int inProgram) const
{
    const juce::ValueTree node = _nodeForProgram(inProgram, false);

    return node.isValid() ? static_cast<float>(node.getProperty(NnId::GainDbId, 0.0)) : 0.0f;
}

bool InstrumentMixer::isMuted(int inProgram) const
{
    const juce::ValueTree node = _nodeForProgram(inProgram, false);

    return node.isValid() && static_cast<bool>(node.getProperty(NnId::MutedId, false));
}

bool InstrumentMixer::isSoloed(int inProgram) const
{
    const juce::ValueTree node = _nodeForProgram(inProgram, false);

    return node.isValid() && static_cast<bool>(node.getProperty(NnId::SoloedId, false));
}

bool InstrumentMixer::isAudible(int inProgram) const
{
    if (isMuted(inProgram)) {
        return false;
    }

    // Solo is only meaningful against the instruments on screen: one left soloed by a previous
    // transcription would otherwise silence the whole of the current one.
    const bool anySoloed =
        std::any_of(mEntries.begin(), mEntries.end(), [this](const InstrumentEntry& e) { return isSoloed(e.program); });

    return !anySoloed || isSoloed(inProgram);
}

void InstrumentMixer::setGainDb(int inProgram, float inGainDb)
{
    const float clamped = juce::jlimit(MIN_INF_GAIN_DB, MAX_GAIN_DB, inGainDb);

    _nodeForProgram(inProgram, true).setProperty(NnId::GainDbId, clamped, nullptr);
}

void InstrumentMixer::setMuted(int inProgram, bool inMuted)
{
    _nodeForProgram(inProgram, true).setProperty(NnId::MutedId, inMuted, nullptr);
}

void InstrumentMixer::setSoloed(int inProgram, bool inSoloed)
{
    _nodeForProgram(inProgram, true).setProperty(NnId::SoloedId, inSoloed, nullptr);
}

void InstrumentMixer::valueTreePropertyChanged(juce::ValueTree& inTree, const juce::Identifier& inProperty)
{
    if (!inTree.hasType(NnId::InstrumentId)) {
        return;
    }

    if (inProperty != NnId::GainDbId && inProperty != NnId::MutedId && inProperty != NnId::SoloedId) {
        return;
    }

    _pushAllToSynth();

    // Gain deliberately does not broadcast. Nothing listening draws it: the piano roll dims notes
    // by audibility, the status bar counts them, and the strip that owns the fader repaints itself.
    // Broadcasting would put a full redraw of every note behind each frame of a fader drag.
    if (inProperty != NnId::GainDbId) {
        sendChangeMessage();
    }
}

void InstrumentMixer::valueTreeChildAdded(juce::ValueTree& inParent, juce::ValueTree& inChild)
{
    juce::ignoreUnused(inParent);

    if (inChild.hasType(NnId::InstrumentMixerId) || inChild.hasType(NnId::InstrumentId)) {
        _pushAllToSynth();
        sendChangeMessage();
    }
}

void InstrumentMixer::valueTreeChildRemoved(juce::ValueTree& inParent, juce::ValueTree& inChild, int inIndex)
{
    juce::ignoreUnused(inParent, inIndex);

    if (inChild.hasType(NnId::InstrumentMixerId) || inChild.hasType(NnId::InstrumentId)) {
        _pushAllToSynth();
        sendChangeMessage();
    }
}

juce::ValueTree InstrumentMixer::_nodeForProgram(int inProgram, bool inCreateIfMissing) const
{
    juce::ValueTree& state = mProcessor->getValueTree();
    juce::ValueTree mixer = state.getChildWithName(NnId::InstrumentMixerId);

    if (!mixer.isValid()) {
        if (!inCreateIfMissing) {
            return {};
        }

        mixer = juce::ValueTree(NnId::InstrumentMixerId);
        state.appendChild(mixer, nullptr);
    }

    juce::ValueTree node = mixer.getChildWithProperty(NnId::ProgramId, inProgram);

    if (!node.isValid() && inCreateIfMissing) {
        node = juce::ValueTree(NnId::InstrumentId);
        node.setProperty(NnId::ProgramId, inProgram, nullptr);
        mixer.appendChild(node, nullptr);
    }

    return node;
}

void InstrumentMixer::_pushAllToSynth()
{
    InstrumentSynth* synth = mProcessor->getPlayer()->getInstrumentSynth();

    for (const InstrumentEntry& entry: mEntries) {
        synth->setGainDb(entry.program, getGainDb(entry.program));
        synth->setMuted(entry.program, isMuted(entry.program));
        synth->setSoloed(entry.program, isSoloed(entry.program));
    }
}
