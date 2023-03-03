#pragma once

#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>

namespace Helpers
{
inline juce::Colour getBackgroundColor()
{
    auto colorID = juce::ResizableWindow::backgroundColourId;

    return juce::LookAndFeel::getDefaultLookAndFeel().findColour(colorID);
}

inline juce::Array<juce::StandalonePluginHolder::PluginInOuts> getChannelConfigurations()
{
#ifdef JucePlugin_PreferredChannelConfigurations
    juce::StandalonePluginHolder::PluginInOuts channels[] = {
        JucePlugin_PreferredChannelConfigurations};

    return juce::Array<juce::StandalonePluginHolder::PluginInOuts>(
        channels, juce::numElementsInArray(channels));
#else
    return {};
#endif
}

inline bool shouldAutoOpenMidiDevices()
{
#if JUCE_DONT_AUTO_OPEN_MIDI_DEVICES_ON_MOBILE
    return false;
#endif
    return true;
}

inline bool shouldUseKioskMode()
{
#if JUCE_STANDALONE_FILTER_WINDOW_USE_KIOSK_MODE
    return true;
#endif
    return false;
}

} // namespace Helpers