//
// Created by Damien Ronssin on 01.06.2024.
//

#ifndef ParameterHelpers_h
#define ParameterHelpers_h

#include <JuceHeader.h>
#include "NoteUtils.h"
#include "RhythmUtils.h"

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
    MinMidiNoteId,
    MaxMidiNoteId,
    KeyRootNoteId,
    KeyTypeId,
    KeySnapModeId,
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
                                     "MIN_MIDI_NOTE",
                                     "MAX_MIDI_NOTE",
                                     "KEY_ROOT_NOTE",
                                     "KEY_TYPE",
                                     "KEY_SNAP_MODE",
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
        case TimeDivisionId:
            return "Time Division";
        case QuantizationForceId:
            return "Quantization Force";
        default:
            jassertfalse;
            return "Unknown";
    }
}

inline String toIdStr(ParamIdEnum id)
{
    return ParamIdStr[id];
}

inline ParameterID toJuceParameterID(ParamIdEnum id)
{
    return {toIdStr(id), versionHint};
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
        case MinMidiNoteId:
            return std::make_unique<AudioParameterInt>(
                toJuceParameterID(id), toName(id), MIN_MIDI_NOTE, MAX_MIDI_NOTE - 12, MIN_MIDI_NOTE);
        case MaxMidiNoteId:
            return std::make_unique<AudioParameterInt>(
                toJuceParameterID(id), toName(id), MIN_MIDI_NOTE + 12, MAX_MIDI_NOTE, MAX_MIDI_NOTE);
        case KeyRootNoteId:
            return std::make_unique<AudioParameterInt>(toJuceParameterID(id), toName(id), 0, 11, 0);
        case KeyTypeId:
            return std::make_unique<AudioParameterChoice>(
                toJuceParameterID(id), toName(id), NoteUtils::ScaleTypesStr, 0);
        case KeySnapModeId:
            return std::make_unique<AudioParameterChoice>(
                toJuceParameterID(id), toName(id), NoteUtils::SnapModesStr, 0);
        case TimeDivisionId:
            return std::make_unique<AudioParameterChoice>(
                toJuceParameterID(id), toName(id), RhythmUtils::TimeDivisionsStr, 0);
        case QuantizationForceId:
            return std::make_unique<AudioParameterFloat>(toJuceParameterID(id), toName(id), 0.0f, 1.0f, 0.5f);

        default:
            jassertfalse;
            return nullptr;
    }
}
} // namespace ParameterHelpers

#endif //ParameterHelpers_h