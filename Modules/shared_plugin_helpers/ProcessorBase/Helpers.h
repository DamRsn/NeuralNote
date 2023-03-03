#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace PluginHelpers
{
//A little helper to get the parameter ID
inline juce::String getParamID(juce::AudioProcessorParameter* param)
{
    if (auto paramWithID = dynamic_cast<juce::AudioProcessorParameterWithID*>(param))
        return paramWithID->paramID;

    return param->getName(50);
}
} // namespace PluginHelpers