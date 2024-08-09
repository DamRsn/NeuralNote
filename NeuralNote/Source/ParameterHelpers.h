//
// Created by Damien Ronssin on 01.06.2024.
//

#ifndef ParameterHelpers_h
#define ParameterHelpers_h

#include <JuceHeader.h>
#include "NoteUtils.h"
#include "TimeQuantizeUtils.h"
#include "NnId.h"

namespace ParameterHelpers
{

static constexpr int versionHint = 1;

enum ParamIdEnum {
    MuteId = 0,
    NoteSensibilityId,
    SplitSensibilityId,
    MinimumNoteDurationId,
    PitchBendModeId,
    AudioPlayerGainId,
    MidiPlayerGainId,
    EnableNoteQuantizationId,
    MinMidiNoteId,
    MaxMidiNoteId,
    KeyRootNoteId,
    KeyTypeId,
    KeySnapModeId,
    EnableTimeQuantizationId,
    TimeDivisionId,
    QuantizationForceId,
    TotalNumParams
};

static const StringArray ParamIdStr {"MUTE",
                                     "NOTE_SENSIBILITY",
                                     "SPLIT_SENSIBILITY",
                                     "MINIMUM_NOTE_DURATION",
                                     "PITCH_BEND_MODE",
                                     "AUDIO_PLAYER_GAIN",
                                     "MIDI_PLAYER_GAIN",
                                     "ENABLE_NOTE_QUANTIZATION",
                                     "MIN_MIDI_NOTE",
                                     "MAX_MIDI_NOTE",
                                     "KEY_ROOT_NOTE",
                                     "KEY_TYPE",
                                     "KEY_SNAP_MODE",
                                     "ENABLE_TIME_QUANTIZATION",
                                     "TIME_DIVISION",
                                     "QUANTIZATION_FORCE"};

inline String toName(ParamIdEnum id)
{
    switch (id) {
        case MuteId:
            return "Mute";
        case NoteSensibilityId:
            return "Note Sensibility";
        case SplitSensibilityId:
            return "Split Sensibility";
        case MinimumNoteDurationId:
            return "Min Note Duration";
        case PitchBendModeId:
            return "Pitch Bend Mode";
        case AudioPlayerGainId:
            return "Audio Level";
        case EnableNoteQuantizationId:
            return "Enable Note Quantization";
        case MidiPlayerGainId:
            return "Midi Level";
        case MinMidiNoteId:
            return "Min Midi Note";
        case MaxMidiNoteId:
            return "Max Midi Note";
        case KeyRootNoteId:
            return "Key Root Note";
        case KeyTypeId:
            return "Key Type";
        case KeySnapModeId:
            return "Key Snap Mode";
        case EnableTimeQuantizationId:
            return "Enable Time Quantization";
        case TimeDivisionId:
            return "Time Division";
        case QuantizationForceId:
            return "Quantization Force";
        default:
            jassertfalse;
            return "Unknown";
    }
}

inline const String& getIdStr(ParamIdEnum id)
{
    return ParamIdStr[id];
}

inline ParameterID toJuceParameterID(ParamIdEnum id)
{
    return {getIdStr(id), versionHint};
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
        case NoteSensibilityId:
            return std::make_unique<AudioParameterFloat>(
                toJuceParameterID(id), toName(id), NormalisableRange<float>(0.05f, 0.95f, 0.01f), 0.7f);
        case SplitSensibilityId:
            return std::make_unique<AudioParameterFloat>(
                toJuceParameterID(id), toName(id), NormalisableRange<float>(0.05f, 0.95f, 0.01f), 0.5f);
        case MinimumNoteDurationId:
            return std::make_unique<AudioParameterFloat>(
                toJuceParameterID(id), toName(id), NormalisableRange<float>(35.0f, 580.0f, 1.0f), 125.0f);
        case PitchBendModeId:
            return std::make_unique<AudioParameterChoice>(
                toJuceParameterID(id), toName(id), StringArray {"No Pitch Bend", "Single Pitch Bend"}, 0);
        case AudioPlayerGainId:
        case MidiPlayerGainId:
            return std::make_unique<AudioParameterFloat>(
                toJuceParameterID(id), toName(id), NormalisableRange<float>(-36.f, 6.0f, 1.0f), 0.0f);
        case EnableNoteQuantizationId:
            return std::make_unique<AudioParameterBool>(toJuceParameterID(id), toName(id), false);
        case MinMidiNoteId:
            return std::make_unique<AudioParameterInt>(
                toJuceParameterID(id), toName(id), MIN_MIDI_NOTE, MAX_MIDI_NOTE, MIN_MIDI_NOTE);
        case MaxMidiNoteId:
            return std::make_unique<AudioParameterInt>(
                toJuceParameterID(id), toName(id), MIN_MIDI_NOTE, MAX_MIDI_NOTE, MAX_MIDI_NOTE);
        case KeyRootNoteId:
            return std::make_unique<AudioParameterInt>(toJuceParameterID(id), toName(id), 0, 11, 3);
        case KeyTypeId:
            return std::make_unique<AudioParameterChoice>(
                toJuceParameterID(id), toName(id), NoteUtils::ScaleTypesStr, 0);
        case KeySnapModeId:
            return std::make_unique<AudioParameterChoice>(
                toJuceParameterID(id), toName(id), NoteUtils::SnapModesStr, 0);
        case EnableTimeQuantizationId:
            return std::make_unique<AudioParameterBool>(toJuceParameterID(id), toName(id), false);
        case TimeDivisionId:
            return std::make_unique<AudioParameterChoice>(
                toJuceParameterID(id), toName(id), TimeQuantizeUtils::TimeDivisionsStr, 5);
        case QuantizationForceId:
            return std::make_unique<AudioParameterFloat>(toJuceParameterID(id), toName(id), 0.0f, 1.0f, 0.f);

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

                int index = ParameterHelpers::ParamIdStr.indexOf(param_id);

                if (index >= 0) {
                    auto* param = inParams[index];
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
