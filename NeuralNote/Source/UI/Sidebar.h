//
// Created by Damien Ronssin on 07.08.26.
//

#ifndef Sidebar_h
#define Sidebar_h

#include <cstdint>
#include <memory>
#include <vector>

#include <JuceHeader.h>

#include "InstrumentStrip.h"
#include "NnFlatButton.h"
#include "NnLevelMeter.h"

class NeuralNoteAudioProcessor;

/**
 * The instrument mixer down the left of the window: a header, one strip per instrument the
 * transcription contains, and the master panel pinned to the bottom.
 */
class Sidebar
    : public juce::Component
    , private juce::ChangeListener
    , private juce::ValueTree::Listener
{
public:
    explicit Sidebar(NeuralNoteAudioProcessor& inProcessor);

    ~Sidebar() override;

    void paint(juce::Graphics& g) override;

    void resized() override;

    /** Re-derives whether the instrument picker can be opened, from the processor's state. */
    void updateEnablements();

    /**
     * Where the picker's top-right corner belongs, in this component's coordinates.
     *
     * The menu itself is owned by the main view rather than by the sidebar: it is wider than the
     * space left of the strips, and its scrim has to cover the whole editor.
     */
    juce::Point<int> getMenuAnchor() const;

    /** Fired by the "+". The main view decides what opening the picker means. */
    std::function<void()> onAddInstrument;

    /** Lights the "+" while its menu is on screen. */
    void setMenuOpen(bool inIsOpen);

private:
    /** The scrollable list itself. Sized to its strips so the viewport can scroll it. */
    class StripList : public juce::Component
    {
    public:
        void resized() override;

        std::vector<std::unique_ptr<InstrumentStrip>> strips;
    };

    void changeListenerCallback(juce::ChangeBroadcaster* inSource) override;

    void valueTreePropertyChanged(juce::ValueTree& inTree, const juce::Identifier& inProperty) override;

    /** Creates or destroys strips so that one exists per instrument, in the mixer's order. */
    void _rebuildStrips();

    /** Pushes the stored selection into the mixer, which is what puts a strip on screen for it. */
    void _pushSelectionToMixer();

    void _paintHeader(juce::Graphics& g, juce::Rectangle<int> inBounds) const;

    /**
     * Drives every meter in the sidebar. One attachment rather than one per strip: they all want
     * the same frame and the same delta, and seventeen listeners would each re-derive it.
     */
    void _onVBlankCallback(double inTimestampSeconds);

    NeuralNoteAudioProcessor& mProcessor;

    juce::Viewport mViewport;
    StripList mStripList;

    NnFlatButton mAddButton {"+"};
    NnLevelMeter mMasterMeter {nn::metrics::masterMeterSegs, nn::metrics::masterMeterGap};

    juce::Rectangle<int> mHeaderBounds;
    juce::Rectangle<int> mMasterBounds;

    // Negative until the first frame, which therefore contributes no release.
    double mLastFrameSeconds = -1.0;

    // When the audio thread last published, so a bypassed plugin's meters fall rather than stick.
    double mLastMeterFrameSeconds = 0.0;
    std::uint32_t mLastMeterFrame = 0;

    // Declared last so it is destroyed first and cannot fire into a half-destroyed sidebar.
    juce::VBlankAttachment mVBlankAttachment {this, [this](double t) { _onVBlankCallback(t); }};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Sidebar)
};

#endif // Sidebar_h
