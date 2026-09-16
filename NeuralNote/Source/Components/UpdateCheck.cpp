//
// Created by Damien Ronssin on 11/11/2024.
//

#include "UpdateCheck.h"

#include "NnFonts.h"
#include "NnIcons.h"
#include "NnLook.h"

namespace
{
/** Between the message, the button and the cross. */
constexpr int CONTENT_GAP = 9;

constexpr int BUTTON_HEIGHT = 24;

/** Compares dotted versions numerically, ignoring a leading "v"; missing components count as 0. */
bool isNewerVersion(const juce::String& inCandidate, const juce::String& inCurrent)
{
    const auto components = [](const juce::String& inVersion) {
        return juce::StringArray::fromTokens(inVersion.trim().trimCharactersAtStart("vV"), ".", "");
    };

    const auto candidate = components(inCandidate);
    const auto current = components(inCurrent);

    for (int i = 0; i < juce::jmax(candidate.size(), current.size()); ++i) {
        const int a = candidate[i].getIntValue();
        const int b = current[i].getIntValue();

        if (a != b) {
            return a > b;
        }
    }

    return false;
}
} // namespace

UpdateCheck::UpdateCheck()
{
    mSeeUpdateButton.setLabel("See update", nn::fonts::buttonLabel());
    mSeeUpdateButton.setPadding(12, 12, 0);

    // Accent-outlined, like Drag MIDI out: it is the one thing this notification is asking for.
    mSeeUpdateButton.setColour(NnFlatButton::backgroundColourId, nn::colours::accentFillButton());
    mSeeUpdateButton.setColour(NnFlatButton::outlineColourId, nn::colours::accent);
    mSeeUpdateButton.setColour(NnFlatButton::textColourId, nn::colours::accentText);
    mSeeUpdateButton.setWantsKeyboardFocus(false);
    mSeeUpdateButton.onClick = [this] { mLatestReleaseUrl.launchInDefaultBrowser(); };
    addChildComponent(mSeeUpdateButton);

    // The same cross the status bar cancels a transcription with, for the same reason: the ten
    // second timer is generous, and there is no reason to wait it out.
    mDismissButton.setIcon(nn::icons::crossStroked, NnFlatButton::IconStyle::stroked, nn::metrics::cancelGlyphSize);
    mDismissButton.setCornerRadius(4.0f);
    mDismissButton.setTooltip("Dismiss");
    mDismissButton.setWantsKeyboardFocus(false);
    mDismissButton.onClick = [this] { _hideNotification(); };
    addAndMakeVisible(mDismissButton);
}

juce::String UpdateCheck::_message() const
{
    return mUpdateAvailable ? "A new version of NeuralNote is available"
                            : "You are on the latest version of NeuralNote";
}

juce::Rectangle<int> UpdateCheck::_panelBounds() const
{
    int width = 2 * nn::metrics::menuPadX + juce::GlyphArrangement::getStringWidthInt(nn::fonts::menuItem(), _message())
                + CONTENT_GAP + nn::metrics::cancelHitSize;

    if (mUpdateAvailable) {
        width += CONTENT_GAP + mSeeUpdateButton.getIdealWidth();
    }

    return getLocalBounds().removeFromRight(juce::jmin(width, getWidth()));
}

void UpdateCheck::resized()
{
    mPanel = _panelBounds();

    auto row = mPanel.reduced(nn::metrics::menuPadX, 0);

    mDismissButton.setBounds(
        row.removeFromRight(nn::metrics::cancelHitSize)
            .withSizeKeepingCentre(nn::metrics::cancelHitSize, nn::metrics::cancelHitSize));

    if (mUpdateAvailable) {
        row.removeFromRight(CONTENT_GAP);

        const int button_width = mSeeUpdateButton.getIdealWidth();
        mSeeUpdateButton.setBounds(
            row.removeFromRight(button_width).withSizeKeepingCentre(button_width, BUTTON_HEIGHT));
    }

    // What the buttons left behind, so a long message ellipsises rather than running under them.
    mTextArea = row.withTrimmedRight(CONTENT_GAP);
}

bool UpdateCheck::hitTest(int inX, int inY)
{
    // The component is given more room than the panel fills, so that the panel can size itself to
    // its message. Without this the empty part of it would swallow clicks meant for the piano roll.
    return mPanel.contains(inX, inY);
}

void UpdateCheck::paint(juce::Graphics& g)
{
    nn::drawPopupSurface(g, mPanel.toFloat());

    g.setColour(nn::colours::popupItem);
    g.setFont(nn::fonts::menuItem());
    g.drawText(_message(), mTextArea, juce::Justification::centredLeft, true);
}

void UpdateCheck::timerCallback()
{
    auto current_time = juce::Time::getCurrentTime();
    auto mouse_over = isMouseOver(true);

    if (mouse_over) {
        mHideTime = std::max(current_time + juce::RelativeTime::seconds(mTimeIncrementOnMouseOverSeconds), mHideTime);
    }

    if (current_time >= mHideTime) {
        _hideNotification();
    }
}

void UpdateCheck::checkForUpdate(bool inShowNotificationOnLatestVersion)
{
    // Nothing here can be cancelled, and the editor can close mid-request: every hop back to the
    // message thread has to check the component is still there.
    const juce::Component::SafePointer<UpdateCheck> safe_this(this);

    // Call async because of issue on Windows with spinning cursor.
    juce::MessageManager::callAsync([safe_this, inShowNotificationOnLatestVersion] {
        if (safe_this == nullptr) {
            return;
        }

        juce::Thread::launch([safe_this, inShowNotificationOnLatestVersion] {
            const juce::URL url("https://api.github.com/repos/DamRsn/NeuralNote/releases/latest");

            const auto result = url.readEntireTextStream();

            if (result.isEmpty()) {
                return;
            }

            auto json = juce::JSON::parse(result);

            if (json.isObject()) {
                const auto current_version = juce::String(JucePlugin_VersionString);

                // Uncomment this line to test the new version available notification
                // const auto current_version = juce::String("0.0.1");

                const auto latest_version = json.getProperty("tag_name", {}).toString();

                juce::MessageManager::callAsync(
                    [current_version, latest_version, inShowNotificationOnLatestVersion, safe_this] {
                        if (safe_this == nullptr) {
                            return;
                        }

                        if (isNewerVersion(latest_version, current_version)) {
                            safe_this->_showNewVersionAvailableNotification();
                        } else if (inShowNotificationOnLatestVersion) {
                            safe_this->_showOnLatestVersionNotification();
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
    mSeeUpdateButton.setVisible(true);
    mHideTime = juce::Time::getCurrentTime() + juce::RelativeTime::seconds(mNotificationDurationSeconds);

    // The panel is sized to its contents, and it just gained a button.
    resized();

    startTimerHz(5);
}

void UpdateCheck::_showOnLatestVersionNotification()
{
    mUpdateAvailable = false;
    setVisible(true);
    mSeeUpdateButton.setVisible(false);
    mHideTime = juce::Time::getCurrentTime() + juce::RelativeTime::seconds(mNotificationDurationSeconds);

    resized();

    startTimerHz(5);
}

void UpdateCheck::_hideNotification()
{
    stopTimer();
    mSeeUpdateButton.setVisible(false);
    setVisible(false);
}
