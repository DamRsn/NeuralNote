//
// Created by Damien Ronssin on 03.08.26.
//

#ifndef TranscriptionConstants_h
#define TranscriptionConstants_h

#include <array>
#include <string_view>

// Must match msl::Transcriber::SAMPLE_RATE (static_assert in MuscriptorEngine.cpp).
// Double, not int: most uses divide a sample count by it to get seconds.
static constexpr double TRANSCRIPTION_SAMPLE_RATE = 16000.0;

// msl::Note carries no velocity (the model only emits onset/offset/pitch), so every note gets this
// fixed synthetic one, mirroring the reference implementation's own convention of hardcoding a
// velocity rather than fabricating one that would look like real data.
static constexpr double FIXED_NOTE_AMPLITUDE = 100.0 / 127.0;

static constexpr int MIN_MIDI_NOTE = 0;
static constexpr int MAX_MIDI_NOTE = 127;

/** The three MuScriptor checkpoints. Nothing else can be loaded. */
enum class ModelSize { Small, Medium, Large };

static constexpr ModelSize DEFAULT_MODEL_SIZE = ModelSize::Medium;

static constexpr std::array<ModelSize, 3> ALL_MODEL_SIZES = {ModelSize::Small, ModelSize::Medium, ModelSize::Large};

/** The name the converter gives this size, and what global.settings stores. */
inline const char* modelSizeToString(ModelSize inModelSize)
{
    switch (inModelSize) {
        case ModelSize::Small:
            return "small";
        case ModelSize::Medium:
            return "medium";
        case ModelSize::Large:
            return "large";
    }

    return "medium";
}

/** What the UI calls this size. */
inline const char* modelSizeToDisplayName(ModelSize inModelSize)
{
    switch (inModelSize) {
        case ModelSize::Small:
            return "Small";
        case ModelSize::Medium:
            return "Medium";
        case ModelSize::Large:
            return "Large";
    }

    return "Medium";
}

/** Anything modelSizeToString did not write -- including an empty string -- gives inFallback. */
inline ModelSize modelSizeFromString(std::string_view inName, ModelSize inFallback)
{
    for (const ModelSize size: ALL_MODEL_SIZES) {
        if (inName == modelSizeToString(size)) {
            return size;
        }
    }

    return inFallback;
}

#endif // TranscriptionConstants_h
