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

// A MidiOverflowMode, for transcriptions with more melodic instruments than there are channels.
inline static Identifier MidiOverflowModeId = "MIDI_OVERFLOW_MODE";

inline static Identifier ZoomLevelId = "ZOOM_LEVEL";

// The piano roll's vertical zoom, as the slider's normalised position. Negative means the view has
// not been told what to show and picks the zoom that fits the transcription; moving the slider is
// what takes it off that, and Reset Zoom is what puts it back.
inline static Identifier VerticalZoomId = "VERTICAL_ZOOM";

// --------------- Instrument mixer -----------------
// A child tree, one node per program, holding the per-instrument fader, mute and solo.
inline static Identifier InstrumentMixerId = "INSTRUMENT_MIXER";

inline static Identifier InstrumentId = "INSTRUMENT";

inline static Identifier ProgramId = "PROGRAM";

inline static Identifier GainDbId = "GAIN_DB";

inline static Identifier MutedId = "MUTED";

inline static Identifier SoloedId = "SOLOED";

// --------------- Instrument selection -------------
// Which instruments the next transcription should look for, as comma-separated msl group ids.
// Empty lets the model choose. A flat property rather than a child tree, so it rides along with
// every other property through save, restore and defaulting.
inline static Identifier SelectedInstrumentGroupsId = "SELECTED_INSTRUMENT_GROUPS";

// To be set in this specific order
const std::vector<std::pair<Identifier, var>> OrderedStatePropertiesWithDefault = {
    {ExportTempoId, 120.0},
    {MidiOverflowModeId, 0}, // MidiOverflowMode::ReuseChannels
    {SourceAudioNativeSrPathId, String()},
    {PlayheadPositionSecId, 0.0},
    {PlayheadCenteredId, true},
    {ZoomLevelId, 1.0},
    {VerticalZoomId, -1.0},
    {SelectedInstrumentGroupsId, String()}};

} // namespace NnId

#endif //NNID_H
