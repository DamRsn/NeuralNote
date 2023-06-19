//
// Created by Damien Ronssin on 04.03.23.
//

#ifndef NN_CONSTANTS_H
#define NN_CONSTANTS_H

static constexpr int NUM_HARMONICS = 8;
static constexpr int NUM_FREQ_IN = 264;
static constexpr int NUM_FREQ_OUT = 88;
static constexpr double BASIC_PITCH_SAMPLE_RATE = 22050.0;

static constexpr int MIDI_OFFSET = 21;
static constexpr int FFT_HOP = 256;
static constexpr int AUDIO_SAMPLE_RATE = 22050;
static constexpr int MAX_NOTE_IDX = 87;
// duration in seconds of training examples - original 1
static constexpr int AUDIO_WINDOW_LENGTH = 2;

// lowest key on a piano
static constexpr float ANNOTATIONS_BASE_FREQUENCY = 27.5;
static constexpr int CONTOURS_BINS_PER_SEMITONE = 3;

static constexpr int MIN_MIDI_NOTE = 21;
static constexpr int MAX_MIDI_NOTE = 108;

#endif //NN_CONSTANTS_H
