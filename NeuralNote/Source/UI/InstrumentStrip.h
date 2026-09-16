//
// Created by Damien Ronssin on 07.08.26.
//

#ifndef InstrumentStrip_h
#define InstrumentStrip_h

#include <JuceHeader.h>

#include "InstrumentMixer.h"
#include "NnFlatButton.h"
#include "NnFlatSlider.h"
#include "NnLevelMeter.h"
#include "NnLook.h"

/**
 * One instrument's row in the sidebar: colour chip, name, note count and range, mute, solo, a fader
 * and the level meter under it.
 */
class InstrumentStrip : public juce::Component
{
public:
    InstrumentStrip(InstrumentMixer& inMixer, int inProgram);

    int getProgram() const { return mProgram; }

    /** Takes the note count and range from a fresh rebuild. Does not touch the fader. */
    void setEntry(const InstrumentEntry& inEntry);

    /** Pulls the fader, mute and solo back out of the mixer, after an edit or a state reload. */
    void refreshFromMixer();

    /** Driven by the sidebar's vblank, which is what holds the one clock every meter shares. */
    void updateMeter(float inLevelDb, float inDtSeconds) { mMeter.setLevelDb(inLevelDb, inDtSeconds); }

    void paint(juce::Graphics& g) override;

    void resized() override;

private:
    /** Re-derives the fader's colours and the strip's muted opacity. */
    void _updateAppearance();

    InstrumentMixer& mMixer;
    int mProgram;

    InstrumentEntry mEntry;

    NnFlatButton mMuteButton {"M"};
    NnFlatButton mSoloButton {"S"};
    NnFlatSlider mFader;
    NnLevelMeter mMeter {nn::metrics::stripMeterSegs, nn::metrics::stripMeterGap};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InstrumentStrip)
};

#endif // InstrumentStrip_h
