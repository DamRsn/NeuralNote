//
// Created by Damien Ronssin on 07.08.26.
//

#ifndef InstrumentMixer_h
#define InstrumentMixer_h

#include <array>
#include <vector>

#include <JuceHeader.h>

#include "NoteEvent.h"

class NeuralNoteAudioProcessor;

/**
 * One row of the sidebar: an instrument the current transcription contains, or one the user has
 * picked for the next run and which has produced nothing yet -- `noteCount == 0` is what tells
 * them apart.
 */
struct InstrumentEntry {
    int program = 0; // 0-127, or msl::DRUM_PROGRAM
    juce::String name;
    juce::String abbreviation;
    juce::Colour colour;
    int noteCount = 0;
    int lowestPitch = 0;
    int highestPitch = 0;

    bool hasNotes() const { return noteCount > 0; }

    bool operator==(const InstrumentEntry&) const = default;
};

/**
 * What instruments the transcription contains, and the fader, mute and solo for each.
 *
 * The single place a program number becomes a colour. The sidebar chip, the fader fill and every
 * note in the piano roll read the same assignment from here, so they cannot disagree -- which is
 * what two independent palettes would eventually do. The assignment itself is per instrument and
 * lives in InstrumentInfo, so an instrument keeps its colour when a later chunk introduces one
 * with a lower program number.
 *
 * The gain/mute/solo values live in the plugin's ValueTree rather than in this object, so a
 * session restores them for free. Setters only write the tree; the listener below is what pushes
 * the result to InstrumentSynth and tells the UI to redraw, so an edit and a state reload take the
 * same path.
 *
 * Not AudioProcessorParameters: there are 129 possible programs, and 387 automatable parameters
 * for six visible faders is not something to hand a host.
 *
 * Message thread only.
 */
class InstrumentMixer
    : public juce::ChangeBroadcaster
    , private juce::ValueTree::Listener
{
public:
    explicit InstrumentMixer(NeuralNoteAudioProcessor* inProcessor);

    ~InstrumentMixer() override;

    /**
     * Re-derives the instrument list from the current notes. Cheap enough to run per decoded chunk:
     * it is one pass over a list the piano roll redraws in full anyway, and deriving the list from
     * the notes rather than keeping a second record is what stops the two drifting apart.
     */
    void rebuildFromNotes(const std::vector<NoteEvent>& inNotes);

    /**
     * The instruments the user has picked for the next run, which get a strip of their own before
     * there is anything to put in it. Kept separate from the note tallies so that a transcription
     * arriving does not have to know about the selection, or the reverse.
     */
    void setSelectedPrograms(std::vector<int> inPrograms);

    /** Drops the note tallies, keeping any strip the selection puts there. Leaves the stored
        faders alone -- resetting those belongs to whoever starts a new transcription, see
        resetStoredSettings. */
    void clear();

    /**
     * Drops every stored fader, mute and solo, so the next transcription's instruments start
     * neutral. Called when a transcription is launched and nowhere else: a session reload does not
     * go through that path, so a restored mix survives.
     */
    void resetStoredSettings();

    const std::vector<InstrumentEntry>& getEntries() const { return mEntries; }

    /** @return The entry for a program, or nullptr if the transcription does not contain it. */
    const InstrumentEntry* findEntry(int inProgram) const;

    /** @return The instrument's colour, or a neutral grey for a program not in the mix. */
    juce::Colour colourForProgram(int inProgram) const;

    float getGainDb(int inProgram) const;
    bool isMuted(int inProgram) const;
    bool isSoloed(int inProgram) const;

    /**
     * @return Whether the instrument is currently heard: not muted, and either soloed itself or
     *         with nothing else soloed. What the piano roll dims its notes by.
     */
    bool isAudible(int inProgram) const;

    /**
     * Reaches the synth like the other two, but does not send a change message: nothing outside
     * the strip that owns the fader draws a gain, and a full piano-roll redraw per frame of a drag
     * is not worth a readout that strip can repaint itself. A caller other than that strip has to
     * refresh it.
     */
    void setGainDb(int inProgram, float inGainDb);

    void setMuted(int inProgram, bool inMuted);
    void setSoloed(int inProgram, bool inSoloed);

    /** Total notes across every instrument, for the status bar. */
    int getTotalNoteCount() const { return mTotalNoteCount; }

    int getLowestPitch() const { return mLowestPitch; }
    int getHighestPitch() const { return mHighestPitch; }

private:
    /** What one program's notes add up to, kept so the entry list can be rebuilt without them. */
    struct Tally {
        int noteCount = 0;
        int lowestPitch = 127;
        int highestPitch = 0;
    };

    /** Re-derives mEntries from the tallies and the selection, and publishes it if it changed. */
    void _rebuildEntries();

    void valueTreePropertyChanged(juce::ValueTree& inTree, const juce::Identifier& inProperty) override;

    /** A state reload appends a whole mixer subtree rather than setting properties one by one. */
    void valueTreeChildAdded(juce::ValueTree& inParent, juce::ValueTree& inChild) override;

    /** And resetStoredSettings drops that subtree wholesale. */
    void valueTreeChildRemoved(juce::ValueTree& inParent, juce::ValueTree& inChild, int inIndex) override;

    /** The node holding one program's settings, created on first write. */
    juce::ValueTree _nodeForProgram(int inProgram, bool inCreateIfMissing) const;

    /** Pushes every stored value to the synth. Run on any change, and after a state reload. */
    void _pushAllToSynth();

    NeuralNoteAudioProcessor* mProcessor;

    std::vector<InstrumentEntry> mEntries;

    std::array<Tally, NUM_INSTRUMENT_IDS> mTallies {};
    std::vector<int> mSelectedPrograms;

    int mTotalNoteCount = 0;
    int mLowestPitch = 0;
    int mHighestPitch = 0;
};

#endif // InstrumentMixer_h
