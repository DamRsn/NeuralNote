//
// Created by Damien Ronssin on 01/12/2024.
//

#ifndef NEURALNOTETOOLTIPS_H
#define NEURALNOTETOOLTIPS_H

#include <JuceHeader.h>

namespace NeuralNoteTooltips
{
// Controls

const String record = "Record | r";

const String clear = "Clear audio and transcription | Shift + Backspace\nRight-click to clear the transcription only";

const String play_pause = "Play / Pause | Space";

const String back = "Go to start | Shift + Space";

const String center = "Center playhead | c";

const String settings = "Settings";

const String mute = "Mute / Unmute input | m";

const String cancel_transcription = "Cancel transcription";

const String model = "Choose the transcription model, or download another";

const String stop_model_download = "Stop the download. Starting it again resumes where it stopped";

const String transcribe = "Transcribe the loaded audio";

const String load_audio = "Load an audio file";

const String add_instrument = "Restrict the transcription to chosen instruments";

const String export_tempo = "Set export tempo for midi file";

const String source_audio_level = "Set source audio level";

const String internal_synth_level = "Set internal synth level";

} // namespace NeuralNoteTooltips

#endif //NEURALNOTETOOLTIPS_H
