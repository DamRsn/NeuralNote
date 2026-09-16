//
// Created by Damien Ronssin on 12.08.26.
//

#include "InstrumentMenu.h"

#include "InstrumentInfo.h"
#include "InstrumentSelection.h"
#include "NnFonts.h"
#include "NnLook.h"
#include "PluginProcessor.h"

namespace
{
const juce::String TITLE = "ADD INSTRUMENT";
const juce::String FOOTER = "TICK TO INCLUDE IN TRANSCRIPTION";

constexpr float TITLE_TRACKING = 0.13f;
constexpr float FOOTER_TRACKING = 0.04f;

constexpr int SHADOW_RADIUS = 34;
constexpr int SHADOW_DROP = 14;
} // namespace

InstrumentMenu::InstrumentMenu(NeuralNoteAudioProcessor& inProcessor)
    : mRowList(inProcessor, mEntries)
{
    // Nothing is selected by default and that means the model chooses, which is worth naming rather
    // than leaving as the state you get by unticking everything.
    mEntries.push_back({"Automatic (any instrument)", std::nullopt});

    for (const msl::InstrumentGroup group: msl::allInstrumentGroups()) {
        mEntries.push_back({instrumentDisplayForGroup(group).name, group});
    }

    mViewport.setViewedComponent(&mRowList, false);
    mViewport.setScrollBarsShown(true, false);

    auto& scrollbar = mViewport.getVerticalScrollBar();
    scrollbar.setColour(juce::ScrollBar::backgroundColourId, juce::Colours::transparentBlack);
    scrollbar.setColour(juce::ScrollBar::thumbColourId, nn::colours::checkboxBorder);
    scrollbar.setColour(juce::ScrollBar::trackColourId, juce::Colours::transparentBlack);

    addAndMakeVisible(mViewport);

    setWantsKeyboardFocus(true);
}

void InstrumentMenu::setPanelAnchor(juce::Point<int> inTopRight)
{
    mAnchor = inTopRight;
    resized();
}

juce::Rectangle<int> InstrumentMenu::_panelBounds() const
{
    const int list_height = juce::jmin(nn::metrics::menuListMaxHeight, mRowList.getIdealHeight());
    const int height = nn::metrics::menuHeaderHeight + list_height + nn::metrics::menuFooterHeight;

    return {mAnchor.x - nn::metrics::menuWidth, mAnchor.y, nn::metrics::menuWidth, height};
}

void InstrumentMenu::resized()
{
    auto panel = _panelBounds();
    panel.removeFromTop(nn::metrics::menuHeaderHeight);
    panel.removeFromBottom(nn::metrics::menuFooterHeight);

    mViewport.setBounds(panel);
    mRowList.setSize(mViewport.getMaximumVisibleWidth(), mRowList.getIdealHeight());
}

void InstrumentMenu::paint(juce::Graphics& g)
{
    auto panel = _panelBounds();

    juce::DropShadow(nn::colours::popupShadow(), SHADOW_RADIUS, {0, SHADOW_DROP}).drawForRectangle(g, panel);

    g.setColour(nn::colours::popupBg);
    g.fillRoundedRectangle(panel.toFloat(), nn::metrics::menuCorner);

    const auto header = panel.withHeight(nn::metrics::menuHeaderHeight);
    nn::drawBottomBorder(g, header, nn::colours::divStrong);

    g.setColour(nn::colours::popupTitle);
    nn::drawTrackedText(g,
                        TITLE,
                        nn::fonts::sectionHeader(),
                        header.reduced(nn::metrics::menuPadX, 0).toFloat(),
                        juce::Justification::centredLeft,
                        TITLE_TRACKING);

    const auto footer = panel.removeFromBottom(nn::metrics::menuFooterHeight);

    // Rounded along the bottom only, so its fill follows the panel's corners instead of squaring
    // them off.
    juce::Path footer_shape;
    footer_shape.addRoundedRectangle(footer.toFloat().getX(),
                                     footer.toFloat().getY(),
                                     footer.toFloat().getWidth(),
                                     footer.toFloat().getHeight(),
                                     nn::metrics::menuCorner,
                                     nn::metrics::menuCorner,
                                     false,
                                     false,
                                     true,
                                     true);

    g.setColour(nn::colours::popupFooterBg);
    g.fillPath(footer_shape);

    nn::drawTopBorder(g, footer, nn::colours::divStrong);

    g.setColour(nn::colours::textFaintest);
    nn::drawTrackedText(g,
                        FOOTER,
                        nn::fonts::meta(),
                        footer.reduced(nn::metrics::menuPadX, 0).toFloat(),
                        juce::Justification::centredLeft,
                        FOOTER_TRACKING);

    g.setColour(nn::colours::popupBorder);
    g.drawRoundedRectangle(_panelBounds().toFloat().reduced(0.5f), nn::metrics::menuCorner, 1.0f);
}

void InstrumentMenu::mouseDown(const juce::MouseEvent& inEvent)
{
    // Everything outside the panel is the scrim. Clicks on the header and the footer land here too,
    // and are meant to do nothing.
    if (!_panelBounds().contains(inEvent.getPosition()) && onDismiss != nullptr) {
        onDismiss();
    }
}

bool InstrumentMenu::keyPressed(const juce::KeyPress& inKey)
{
    if (inKey == juce::KeyPress::escapeKey && onDismiss != nullptr) {
        onDismiss();
        return true;
    }

    return false;
}

void InstrumentMenu::visibilityChanged()
{
    if (isVisible()) {
        // Taken so Escape reaches this rather than the main view's transport shortcuts. The main
        // view takes it back when the menu closes.
        grabKeyboardFocus();
    }
}

InstrumentMenu::RowList::RowList(NeuralNoteAudioProcessor& inProcessor, const std::vector<Entry>& inEntries)
    : mProcessor(inProcessor)
    , mEntries(inEntries)
{
}

int InstrumentMenu::RowList::getIdealHeight() const
{
    return static_cast<int>(mEntries.size()) * nn::metrics::menuRowHeight + 2 * nn::metrics::menuListPadY;
}

int InstrumentMenu::RowList::_rowAt(juce::Point<int> inPosition) const
{
    // Guarded rather than left to the division: negative offsets truncate toward zero, which would
    // put the top padding on the first row.
    if (inPosition.y < nn::metrics::menuListPadY) {
        return -1;
    }

    const int row = (inPosition.y - nn::metrics::menuListPadY) / nn::metrics::menuRowHeight;

    return juce::isPositiveAndBelow(row, static_cast<int>(mEntries.size())) ? row : -1;
}

void InstrumentMenu::RowList::paint(juce::Graphics& g)
{
    const std::vector<msl::InstrumentGroup> selected = InstrumentSelection::get(mProcessor.getValueTree());

    int y = nn::metrics::menuListPadY;

    for (int i = 0; i < static_cast<int>(mEntries.size()); i++) {
        const Entry& entry = mEntries[static_cast<std::size_t>(i)];

        const bool ticked = entry.group.has_value()
                                ? std::find(selected.begin(), selected.end(), *entry.group) != selected.end()
                                : selected.empty();

        auto row = juce::Rectangle<int>(0, y, getWidth(), nn::metrics::menuRowHeight);
        y += nn::metrics::menuRowHeight;

        if (i == mHoveredRow) {
            g.setColour(nn::colours::popupRowHover);
            g.fillRect(row);
        }

        row = row.reduced(nn::metrics::menuPadX, 0);

        nn::drawCheckbox(g, row.removeFromRight(nn::metrics::checkboxSize), ticked);

        g.setColour(ticked ? nn::colours::popupItemTicked : nn::colours::popupItem);
        g.setFont(ticked ? nn::fonts::menuItemTicked() : nn::fonts::menuItem());
        g.drawText(entry.name, row.withTrimmedRight(nn::metrics::menuPadX), juce::Justification::centredLeft, true);
    }
}

void InstrumentMenu::RowList::mouseMove(const juce::MouseEvent& inEvent)
{
    const int row = _rowAt(inEvent.getPosition());

    if (row != mHoveredRow) {
        mHoveredRow = row;
        repaint();
    }
}

void InstrumentMenu::RowList::mouseExit(const juce::MouseEvent& inEvent)
{
    juce::ignoreUnused(inEvent);

    if (mHoveredRow != -1) {
        mHoveredRow = -1;
        repaint();
    }
}

void InstrumentMenu::RowList::mouseDown(const juce::MouseEvent& inEvent)
{
    const int row = _rowAt(inEvent.getPosition());

    if (row < 0) {
        return;
    }

    juce::ValueTree& state = mProcessor.getValueTree();
    const std::optional<msl::InstrumentGroup> group = mEntries[static_cast<std::size_t>(row)].group;

    if (group.has_value()) {
        InstrumentSelection::toggle(state, *group);
    } else {
        InstrumentSelection::clear(state);
    }

    // The menu stays open, so the row that was just ticked has to redraw itself here.
    repaint();
}
