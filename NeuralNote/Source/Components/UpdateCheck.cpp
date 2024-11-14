//
// Created by Damien Ronssin on 11/11/2024.
//

#include "UpdateCheck.h"

#include "UIDefines.h"

UpdateCheck::UpdateCheck()
{
    mUrlButton.setButtonText("See update");
    mUrlButton.setURL(mLatestReleaseUrl);
    mUrlButton.setFont(LABEL_FONT, false);
    mUrlButton.setJustificationType(Justification::centred);
    mUrlButton.setColour(HyperlinkButton::ColourIds::textColourId, Colours::blue);
    addAndMakeVisible(mUrlButton);
}

void UpdateCheck::resized()
{
    mUrlButton.setBounds(215, 0, 70, getHeight());
}

void UpdateCheck::paint(Graphics& g)
{
    auto left_offset = mUpdateAvailable ? 0 : 48;
    g.setColour(WHITE_SOLID);
    g.fillRoundedRectangle(getLocalBounds().toFloat().withLeft(static_cast<float>(left_offset)), 4.0f);

    g.setColour(BLACK);
    g.setFont(LABEL_FONT);

    if (mUpdateAvailable) {
        g.drawText("A new version of NeuralNote is available:",
                   Rectangle<int>({5 + left_offset, 0, 210, getHeight()}),
                   Justification::centredLeft);
    } else {
        g.drawText("You are on the latest version of NeuralNote!",
                   Rectangle<int>({5 + left_offset, 0, 250, getHeight()}),
                   Justification::centredLeft);
    }
}

void UpdateCheck::timerCallback()
{
    auto current_time = Time::getCurrentTime();
    auto mouse_over = isMouseOver(true);

    if (mouse_over) {
        mHideTime = std::max(current_time + RelativeTime::seconds(mTimeIncrementOnMouseOverSeconds), mHideTime);
    }

    if (current_time >= mHideTime) {
        _hideNotification();
    }
}

void UpdateCheck::checkForUpdate(bool inShowNotificationOnLatestVersion)
{
    Thread::launch([this, inShowNotificationOnLatestVersion] {
        URL url("https://api.github.com/repos/DamRsn/NeuralNote/releases/latest");

        auto result = url.readEntireTextStream();

        if (result.isEmpty()) {
            return;
        }

        auto json = JSON::parse(result);

        if (json.isObject()) {
            const auto current_version_str = String("v") + String(JucePlugin_VersionString).trim();
            // const auto current_version_str = String("v0.0.1");
            auto latest_version = json.getProperty("tag_name", "unknown").toString().trim();

            MessageManager::callAsync([current_version_str, latest_version, inShowNotificationOnLatestVersion, this] {
                if (!current_version_str.equalsIgnoreCase(latest_version)) {
                    _showNewVersionAvailableNotification();
                } else if (inShowNotificationOnLatestVersion) {
                    _showOnLatestVersionNotification();
                }
            });
        } else {
            jassertfalse;
        }
    });
}

void UpdateCheck::_showNewVersionAvailableNotification()
{
    mUpdateAvailable = true;
    setVisible(true);
    mUrlButton.setVisible(true);
    mHideTime = Time::getCurrentTime() + RelativeTime::seconds(mNotificationDurationSeconds);

    startTimerHz(5);
}

void UpdateCheck::_showOnLatestVersionNotification()
{
    mUpdateAvailable = false;
    setVisible(true);
    mUrlButton.setVisible(false);
    mHideTime = Time::getCurrentTime() + RelativeTime::seconds(mNotificationDurationSeconds);

    startTimerHz(5);
}

void UpdateCheck::_hideNotification()
{
    stopTimer();
    mUrlButton.setVisible(false);
    setVisible(false);
}
