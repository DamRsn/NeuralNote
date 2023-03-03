#include "StandaloneFilter.h"
#include "ConfigurationHelpers.h"

#if JUCE_IOS
void* juce_GetIOSCustomDelegateClass()  { return nullptr; }
#endif

StandaloneFilterApp::StandaloneFilterApp()
{
    juce::PluginHostType::jucePlugInClientCurrentWrapperType =
        juce::AudioProcessor::wrapperType_Standalone;

    appProperties.setStorageParameters(PropertiesFileOptions());
}

juce::StandaloneFilterWindow* StandaloneFilterApp::createWindow()
{
    return new juce::StandaloneFilterWindow(getApplicationName(),
                                            Helpers::getBackgroundColor(),
                                            appProperties.getUserSettings(),
                                            false,
                                            {},
                                            nullptr,
                                            Helpers::getChannelConfigurations(),
                                            Helpers::shouldAutoOpenMidiDevices());
}

void StandaloneFilterApp::initialise(const juce::String&)
{
    mainWindow.reset(createWindow());

    if (!Helpers::shouldUseKioskMode())
        mainWindow->setVisible(true);
}

void StandaloneFilterApp::shutdown()
{
    mainWindow = nullptr;
    appProperties.saveIfNeeded();
}

void StandaloneFilterApp::systemRequestedQuit()
{
    if (mainWindow != nullptr)
        mainWindow->pluginHolder->savePluginState();

    if (juce::ModalComponentManager::getInstance()->cancelAllModalComponents())
        juce::Timer::callAfterDelay(100, [&]() { requestQuit(); });
    else
        quit();
}

void StandaloneFilterApp::requestQuit() const
{
    if (auto app = getInstance())
        app->systemRequestedQuit();
}

juce::JUCEApplicationBase* juce_CreateApplication()
{
    return new StandaloneFilterApp();
}
