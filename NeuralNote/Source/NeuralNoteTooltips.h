//
// Created by Damien Ronssin on 01/12/2024.
//

#ifndef NEURALNOTETOOLTIPS_H
#define NEURALNOTETOOLTIPS_H

#include <JuceHeader.h>

namespace NeuralNoteTooltips
{
// Transcription options
const String to_note_sensibility = "Set note sensibility\n"
                                   "Higher values will detect more notes";

const String to_split_sensibility = "Set split sensibility\n"
                                    "Higher values result in more splits\nLower values results in longer held notes";

const String to_min_note_duration = "Set minimum note duration\n"
                                    "Notes shorter than this are removed";

const String to_pitch_bend = "Set pitch bend mode\n"
                             "No Pitch Bend: Transcription will not include any pitch bend\n"
                             "Single Pitch Bend: Transcription will include pitch bend for non-overlapping notes";

// Scale quantization
const String sq_enable = "Enable / Disable scale quantization";

const String sq_note_range = "Set note range";

const String sq_root_note = "Set scale root note";

const String sq_scale_type = "Set scale type";

const String sq_snap_mode = "Set snap mode\nAdjust: snap to closest note in scale\nRemove: remove if not in scale";

// Time quantization
const String tq_enable = "Enable / Disable time quantization";

const String tq_time_division = "Set time division to quantize to";

const String tq_quantization_force = "Set quantization force";

const String tq_tempo = "Set tempo";

const String tq_numerator = "Set time signature numerator";

const String tq_denominator = "Set time signature denominator";

// Controls

const String record = "Record | r";

const String clear = "Clear audio and midi | Shift + Backspace";

const String play_pause = "Play / Pause | Space";

const String back = "Go to start | Shift + Space";

const String center = "Center playhead | c";

const String settings = "Settings";

const String mute = "Mute / Unmute input | m";

const String export_tempo = "Set export tempo for midi file";

const String source_audio_level = "Set source audio level";

const String internal_synth_level = "Set internal synth level";

} // namespace NeuralNoteTooltips

#endif //NEURALNOTETOOLTIPS_H
