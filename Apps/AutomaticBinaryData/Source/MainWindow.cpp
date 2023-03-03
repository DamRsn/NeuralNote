#include "MainWindow.h"

constexpr bool isMobile()
{
#if JUCE_IOS || JUCE_ANDROID
    return true;
#else
    return false;
#endif
}

MainWindow::MainWindow(const String& name)
    : DocumentWindow(name, getBackgroundColour(), allButtons)
{
    setUsingNativeTitleBar(true);
    setContentOwned(new MainComponent(), true);

    if (isMobile())
        setFullScreen(true);
    else
    {
        setResizable(true, true);
        centreWithSize(getWidth(), getHeight());
    }

    setVisible(true);
}

void MainWindow::closeButtonPressed()
{
    JUCEApplication::getInstance()->systemRequestedQuit();
}

Colour MainWindow::getBackgroundColour()
{
    return Desktop::getInstance().getDefaultLookAndFeel().findColour(
        ResizableWindow::backgroundColourId);
}
