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

inline static Identifier DetectedTempoId = "DETECTED_TEMPO";

inline static Identifier DetectedTimeSignatureId = "DETECTED_TIME_SIGNATURE";

inline static Identifier ExportTempoId = "EXPORT_TEMPO";

const std::vector<std::pair<Identifier, var>> OrderedStatePropertiesWithDefault = {
    {SourceAudioNativeSrPathId, String()},
    {PlayheadPositionSecId, 0.0},
    {PlayheadCenteredId, true},
    {DetectedTempoId, 120.0},
    {DetectedTimeSignatureId, "4/4"},
    {ExportTempoId, 120.0}};

} // namespace NnId

#endif //NNID_H
