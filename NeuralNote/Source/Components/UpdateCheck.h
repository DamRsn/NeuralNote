//
// Created by Damien Ronssin on 11/11/2024.
//

#ifndef UPDATECHECK_H
#define UPDATECHECK_H

#include <JuceHeader.h>

#include "NnFlatButton.h"

/**
 * The notification that says whether there is a newer NeuralNote, on the same panel the menus and
 * tooltips sit on. Asks GitHub for the latest release, then shows itself for ten seconds -- longer
 * while the pointer is over it, so it cannot expire out from under someone reading it.
 *
 * Sized to its own contents and pinned to the right of whatever bounds it is given.
 */
class UpdateCheck
    : public juce::Component
    , public juce::Timer
{
public:
    UpdateCheck();

    void resized() override;

    bool hitTest(int inX, int inY) override;

    void paint(juce::Graphics& g) override;

    void timerCallback() override;

    void checkForUpdate(bool inShowNotificationOnLatestVersion);

private:
    void _showNewVersionAvailableNotification();

    void _showOnLatestVersionNotification();

    void _hideNotification();

    juce::String _message() const;

    /** The panel itself: as wide as its contents need, against the right-hand edge. */
    juce::Rectangle<int> _panelBounds() const;

    bool mUpdateAvailable {false};

    juce::Rectangle<int> mPanel;
    juce::Rectangle<int> mTextArea;

    NnFlatButton mSeeUpdateButton {"SeeUpdate"};
    NnFlatButton mDismissButton {"DismissUpdateNotification"};

    juce::Time mHideTime;

    static constexpr double mNotificationDurationSeconds = 10.0f;
    static constexpr double mTimeIncrementOnMouseOverSeconds = 3.0f;

    const juce::URL mLatestReleaseUrl {"https://github.com/DamRsn/NeuralNote/releases/latest"};
};

#endif //UPDATECHECK_H
