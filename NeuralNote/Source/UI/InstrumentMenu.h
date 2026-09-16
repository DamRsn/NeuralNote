//
// Created by Damien Ronssin on 12.08.26.
//

#ifndef InstrumentMenu_h
#define InstrumentMenu_h

#include <optional>
#include <vector>

#include <JuceHeader.h>

#include "muscriptor/note.hpp"

class NeuralNoteAudioProcessor;

/**
 * The picker behind the sidebar's "+": every instrument the model can name, ticked where it is
 * selected, with "Automatic" at the top for the default of letting the model choose.
 *
 * A component rather than a juce::PopupMenu: the selection is a multi-select, so the panel has to
 * stay open across clicks.
 *
 * It covers the whole editor: everything outside the panel is the scrim that closes it.
 */
class InstrumentMenu : public juce::Component
{
public:
    explicit InstrumentMenu(NeuralNoteAudioProcessor& inProcessor);

    /** Puts the panel's top-right corner here, in this component's coordinates. */
    void setPanelAnchor(juce::Point<int> inTopRight);

    /** Fired when the menu closes itself: a click on the scrim, or Escape. */
    std::function<void()> onDismiss;

    void resized() override;

    void paint(juce::Graphics& g) override;

    void mouseDown(const juce::MouseEvent& inEvent) override;

    bool keyPressed(const juce::KeyPress& inKey) override;

    void visibilityChanged() override;

private:
    /** One offer in the list. No group means "Automatic", which is the empty selection. */
    struct Entry {
        juce::String name;
        std::optional<msl::InstrumentGroup> group;
    };

    /** The rows themselves, sized to their content so the viewport can scroll them. */
    class RowList : public juce::Component
    {
    public:
        RowList(NeuralNoteAudioProcessor& inProcessor, const std::vector<Entry>& inEntries);

        int getIdealHeight() const;

        void paint(juce::Graphics& g) override;

        void mouseMove(const juce::MouseEvent& inEvent) override;

        void mouseExit(const juce::MouseEvent& inEvent) override;

        void mouseDown(const juce::MouseEvent& inEvent) override;

    private:
        int _rowAt(juce::Point<int> inPosition) const;

        NeuralNoteAudioProcessor& mProcessor;
        const std::vector<Entry>& mEntries;

        int mHoveredRow = -1;
    };

    juce::Rectangle<int> _panelBounds() const;

    std::vector<Entry> mEntries;

    juce::Viewport mViewport;
    RowList mRowList;

    juce::Point<int> mAnchor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InstrumentMenu)
};

#endif // InstrumentMenu_h
