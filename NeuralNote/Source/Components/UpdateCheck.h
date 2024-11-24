//
// Created by Damien Ronssin on 11/11/2024.
//

#ifndef UPDATECHECK_H
#define UPDATECHECK_H

#include <JuceHeader.h>

class UpdateCheck
    : public Component
    , public Timer
{
public:
    UpdateCheck();

    void resized() override;

    void paint(Graphics& g) override;

    void timerCallback() override;

    void checkForUpdate(bool inShowNotificationOnLatestVersion);

private:
    void _showNewVersionAvailableNotification();

    void _showOnLatestVersionNotification();

    void _hideNotification();

    bool mUpdateAvailable {false};

    HyperlinkButton mUrlButton;

    Time mHideTime;

    static constexpr int mPadding = 5;

    static constexpr double mNotificationDurationSeconds = 10.0f;
    static constexpr double mTimeIncrementOnMouseOverSeconds = 3.0f;

    const URL mLatestReleaseUrl {"https://github.com/DamRsn/NeuralNote/releases/latest"};
};

#endif //UPDATECHECK_H
