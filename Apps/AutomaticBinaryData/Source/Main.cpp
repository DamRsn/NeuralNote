#include "MainWindow.h"

class GuiAppTemplateApplication : public JUCEApplication
{
public:
    const String getApplicationName() override { return "Automatic Binary Data"; }
    const String getApplicationVersion() override { return "0.3"; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const String& /*commandLine*/) override
    {
        mainWindow = std::make_unique<MainWindow>(getApplicationName());
    }

    void shutdown() override { mainWindow.reset(); }

    void systemRequestedQuit() override { quit(); }

    void anotherInstanceStarted(const String& /*commandLine*/) override {}

private:
    std::unique_ptr<MainWindow> mainWindow;
};

// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION(GuiAppTemplateApplication)
