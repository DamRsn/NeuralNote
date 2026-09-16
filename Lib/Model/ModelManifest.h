//
// Created by Damien Ronssin on 12.09.26.
//

#ifndef ModelManifest_h
#define ModelManifest_h

#include <cstdint>

#include "TranscriptionConstants.h"

/**
 * The checkpoints this build downloads, pinned to one commit of the published repo.
 *
 * The digests are compiled in rather than read from the repo's SHA256SUMS, so a file that changed
 * upstream fails verification instead of being trusted. New weights mean a new commit and new
 * entries, changed together.
 */
struct ModelManifestEntry {
    const char* fileName;
    std::int64_t numBytes;
    const char* sha256;
};

static constexpr const char* MODEL_REPO = "DamRsn/muscriptor-gguf";
static constexpr const char* MODEL_REVISION = "d7045f94e8b19427f4ff9542975035e66596e51c";

// Named for the checkpoint format generation, muscriptor.format_version in the GGUF.
static constexpr const char* MODEL_REPO_DIRECTORY = "v1";

inline const ModelManifestEntry& getModelManifestEntry(ModelSize inModelSize)
{
    static constexpr ModelManifestEntry SMALL {
        "muscriptor-small-f16.gguf", 209425152, "925f55af65a20ebc4f8b45ceaf095a12b72493d436cb112623cd0041a1af23d4"};
    static constexpr ModelManifestEntry MEDIUM {
        "muscriptor-medium-f16.gguf", 618442496, "3850cc9e5b436b17a09bd25b8f2615cb3366ab96a71e7b50f73a793a917fdf03"};
    static constexpr ModelManifestEntry LARGE {
        "muscriptor-large-f16.gguf", 2739142176, "35a750fb1ab1e77195cdc2c0b9b4aeea2f4d59f11f729f02af9920c4854ef72e"};

    switch (inModelSize) {
        case ModelSize::Small:
            return SMALL;
        case ModelSize::Medium:
            return MEDIUM;
        case ModelSize::Large:
            return LARGE;
    }

    return MEDIUM;
}

#endif // ModelManifest_h
