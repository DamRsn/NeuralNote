#pragma once

#include "PropertiesFileOptions.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>

extern juce::JUCEApplicationBase* juce_CreateApplication();

class StandaloneFilterApp : public juce::JUCEApplication
{
public:
    StandaloneFilterApp();

private:

    const juce::String getApplicationName() override { return JucePlugin_Name; }
    const juce::String getApplicationVersion() override { return JucePlugin_VersionString; }
    bool moreThanOneInstanceAllowed() override { return true; }
    void anotherInstanceStarted(const juce::String&) override {}

    void initialise(const juce::String&) override;
    void shutdown() override;
    void systemRequestedQuit() override;
    void requestQuit() const;

    juce::StandaloneFilterWindow* createWindow();

    juce::ApplicationProperties appProperties;
    std::unique_ptr<juce::StandaloneFilterWindow> mainWindow;
};


