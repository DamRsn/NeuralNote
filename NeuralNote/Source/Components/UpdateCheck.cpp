//
// Created by Damien Ronssin on 11/11/2024.
//

#include "UpdateCheck.h"

#include "UIDefines.h"

UpdateCheck::UpdateCheck()
{
    mUrlButton.setButtonText("See update");
    mUrlButton.setURL(mLatestReleaseUrl);
    mUrlButton.setFont(UIDefines::LABEL_FONT(), false);
    mUrlButton.setJustificationType(Justification::centred);
    mUrlButton.setColour(HyperlinkButton::ColourIds::textColourId, Colours::blue);
    addAndMakeVisible(mUrlButton);
}

void UpdateCheck::resized()
{
    mUrlButton.setBounds(getWidth() - 65 - mPadding, 0, 65, getHeight());
}

void UpdateCheck::paint(Graphics& g)
{
    g.setColour(WHITE_SOLID);
    g.setFont(UIDefines::LABEL_FONT());

    String text;
    if (mUpdateAvailable) {
        text = "A new version of NeuralNote is available:";
    } else {
        text = "You are on the latest version of NeuralNote!";
    }

    AttributedString attributed_string(text);
    attributed_string.setFont(UIDefines::LABEL_FONT());
    attributed_string.setJustification(Justification::centred);

    TextLayout text_layout;
    text_layout.createLayout(attributed_string, static_cast<float>(getWidth()), static_cast<float>(getHeight()));
    float text_width = text_layout.getWidth();
    float rectangle_width = text_width + 2 * mPadding;

    if (mUpdateAvailable) {
        rectangle_width += static_cast<float>(mUrlButton.getWidth());
    }

    int rect_x_start = getWidth() - static_cast<int>(rectangle_width);

    g.fillRoundedRectangle(getLocalBounds().toFloat().withLeft(static_cast<float>(rect_x_start)), 4.0f);

    g.setColour(BLACK);
    text_layout.draw(
        g,
        Rectangle<float>(static_cast<float>(rect_x_start + mPadding), 0, text_width, static_cast<float>(getHeight())));
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
    // Call async because of issue on Windows with spinning cursor.
    MessageManager::callAsync([this, inShowNotificationOnLatestVersion] {
        Thread::launch([this, inShowNotificationOnLatestVersion] {
            const URL url("https://api.github.com/repos/DamRsn/NeuralNote/releases/latest");

            const auto result = url.readEntireTextStream();

            if (result.isEmpty()) {
                return;
            }

            auto json = JSON::parse(result);

            if (json.isObject()) {
                const auto current_version_str = String("v") + String(JucePlugin_VersionString).trim();

                // Uncomment this line to test the new version available notification
                // const auto current_version_str = String("v0.0.1");

                const auto latest_version = json.getProperty("tag_name", "unknown").toString().trim();

                MessageManager::callAsync(
                    [current_version_str, latest_version, inShowNotificationOnLatestVersion, this] {
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