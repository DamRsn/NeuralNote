//
// Created by Damien Ronssin on 13.07.2024.
//

#ifndef NNID_H
#define NNID_H

#include <JuceHeader.h>

namespace NnId
{
inline static Identifier ValueId = "value";

inline static Identifier IdId = "id";

inline static Identifier FullStateId = "NEURAL_NOTE_FULL_STATE";

inline static Identifier ParametersId = "PARAMETERS";

inline static Identifier NeuralNoteStateId = "NEURAL_NOTE_STATE";

inline static Identifier NeuralNoteVersionId = "NEURAL_NOTE_VERSION";

inline static Identifier SourceAudioNativeSrPathId = "SOURCE_AUDIO_NATIVE_SR_PATH";

inline static Identifier PlayheadPositionSecId = "PLAYHEAD_POSITION_SEC";

inline static Identifier PlayheadCenteredId = "PLAYHEAD_CENTERED";

inline static Identifier MidiOut = "MIDI_OUT";

inline static Identifier ExportTempoId = "EXPORT_TEMPO";

inline static Identifier ZoomLevelId = "ZOOM_LEVEL";

inline static Identifier TooltipVisibleId = "TOOLTIP_VISIBLE";

// --------------- Time quantization ----------------
inline static Identifier TempoId = "TEMPO";

inline static Identifier TimeSignatureNumeratorId = "TIME_SIGNATURE_NUMERATOR";

inline static Identifier TimeSignatureDenominatorId = "TIME_SIGNATURE_DENOMINATOR";

inline static Identifier TimeQuantizeRefPosQnId = "TIME_QUANTIZE_REF_POS_QN";

inline static Identifier TimeQuantizeRefLastBarQnId = "TIME_QUANTIZE_REF_LAST_BAR_QN";

inline static Identifier TimeQuantizeRefPosSec = "TIME_QUANTIZE_REF_POS_SECONDS";

// To be set in this specific order
const std::vector<std::pair<Identifier, var>> OrderedStatePropertiesWithDefault = {
    {TempoId, 120.0},
    {ExportTempoId, 120.0},
    {TimeSignatureNumeratorId, 4},
    {TimeSignatureDenominatorId, 4},
    {TimeQuantizeRefPosQnId, 0.0},
    {TimeQuantizeRefLastBarQnId, 0.0},
    {TimeQuantizeRefPosSec, 0.0},
    {SourceAudioNativeSrPathId, String()},
    {PlayheadPositionSecId, 0.0},
    {PlayheadCenteredId, true},
    {ZoomLevelId, 1.0},
    {MidiOut, false},
    {TooltipVisibleId, true}};

} // namespace NnId

#endif //NNID_H
