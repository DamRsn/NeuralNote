//
// Created by Damien Ronssin on 01.06.2024.
//

#ifndef ParameterHelpers_h
#define ParameterHelpers_h

#include <JuceHeader.h>
#include "GainConstants.h"
#include "NnId.h"

namespace ParameterHelpers
{

enum ParamIdEnum {
    MuteId = 0,
    MixId,
    MasterGainId,
    TotalNumParams
};

static const StringArray ParamIdStr {"MUTE", "MIX", "MASTER_GAIN"};

inline String toName(ParamIdEnum id)
{
    switch (id) {
        case MuteId:
            return "Mute";
        case MixId:
            return "Mix";
        case MasterGainId:
            return "Master Gain";
        case TotalNumParams:
        default:
            jassertfalse;
            return "Unknown";
    }
}

inline const String& getIdStr(ParamIdEnum id)
{
    return ParamIdStr[static_cast<int>(id)];
}

/**
 * The plugin version a parameter was introduced in. JUCE orders the Audio Unit parameter list by
 * it, so moving an existing one costs Logic and GarageBand the automation they saved.
 */
inline int toVersionHint(ParamIdEnum id)
{
    switch (id) {
        case MuteId:
            return 2;
        case MixId:
        case MasterGainId:
            return 3;
        case TotalNumParams:
        default:
            jassertfalse;
            return 1;
    }
}

inline ParameterID toJuceParameterID(ParamIdEnum id)
{
    return {getIdStr(id), toVersionHint(id)};
}

inline float getUnmappedParamValue(RangedAudioParameter* inParam)
{
    return inParam->getNormalisableRange().convertFrom0to1(inParam->getValue());
}

inline std::unique_ptr<RangedAudioParameter> getRangedAudioParamForID(ParamIdEnum id)
{
    switch (id) {
        case MuteId:
            return std::make_unique<AudioParameterBool>(toJuceParameterID(id), toName(id), false);
        case MixId:
            return std::make_unique<AudioParameterFloat>(
                toJuceParameterID(id), toName(id), NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f);
        case MasterGainId:
            return std::make_unique<AudioParameterFloat>(
                toJuceParameterID(id), toName(id), NormalisableRange<float>(MIN_INF_GAIN_DB, MAX_GAIN_DB, 0.1f), 0.0f);
        case TotalNumParams:
        default:
            jassertfalse;
            return nullptr;
    }
}

inline AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    for (size_t i = 0; i < TotalNumParams; i++) {
        auto pid = static_cast<ParamIdEnum>(i);
        params.push_back(getRangedAudioParamForID(pid));
    }

    return {params.begin(), params.end()};
}

inline void updateParametersFromState(const ValueTree& inParameterTree,
                                      std::array<RangedAudioParameter*, TotalNumParams>& inParams)
{
    if (inParameterTree.isValid()) {
        // Iterate through the properties in the loaded state
        for (int i = 0; i < inParameterTree.getNumChildren(); ++i) {
            auto child = inParameterTree.getChild(i);

            if (child.isValid() && child.hasProperty(NnId::IdId) && child.hasProperty(NnId::ValueId)) {
                auto param_id = child.getProperty(NnId::IdId).toString();

                int index = ParamIdStr.indexOf(param_id);

                if (index >= 0) {
                    auto* param = inParams[static_cast<size_t>(index)];
                    auto value = jlimit(param->getNormalisableRange().start,
                                        param->getNormalisableRange().end,
                                        static_cast<float>(child.getProperty(NnId::ValueId)));

                    auto norm_value = param->getNormalisableRange().convertTo0to1(value);
                    param->setValueNotifyingHost(norm_value);
                }
            }
        }
    }
}

} // namespace ParameterHelpers

#endif //ParameterHelpers_h
