//
// Created by Damien Ronssin on 12.09.26.
//

#ifndef ModelManifest_h
#define ModelManifest_h

#include <cstdint>
#include <string>

#include "TranscriptionConstants.h"

static constexpr const char* MODEL_REPO = "DamRsn/muscriptor-gguf";
static constexpr const char* MODEL_REVISION = "d7045f94e8b19427f4ff9542975035e66596e51c";

// Named for the checkpoint format generation, muscriptor.format_version in the GGUF.
static constexpr const char* MODEL_REPO_DIRECTORY = "v1";

/**
 * The checkpoints this build downloads, each pinned to one commit of the repo it is published in.
 *
 * The digests are compiled in rather than read from the repo's SHA256SUMS, so a file that changed
 * upstream fails verification instead of being trusted. New weights mean a new commit and new
 * entries, changed together.
 */
struct ModelManifestEntry {
    const char* fileName;
    std::int64_t numBytes;
    const char* sha256;

    // Where it is published on Hugging Face: MuScriptor's repo, unless the entry names its own.
    const char* repo = MODEL_REPO;
    const char* revision = MODEL_REVISION;
    // Empty for a file at the root of the repo.
    const char* directory = MODEL_REPO_DIRECTORY;
};

inline const ModelManifestEntry& getModelManifestEntry(ModelSize inModelSize)
{
    static constexpr ModelManifestEntry SMALL {
        "muscriptor-small-f16.gguf", 209425152, "925f55af65a20ebc4f8b45ceaf095a12b72493d436cb112623cd0041a1af23d4"};
    static constexpr ModelManifestEntry MEDIUM {
        "muscriptor-medium-f16.gguf", 618442496, "3850cc9e5b436b17a09bd25b8f2615cb3366ab96a71e7b50f73a793a917fdf03"};
    static constexpr ModelManifestEntry LARGE {
        "muscriptor-large-f16.gguf", 2739142176, "35a750fb1ab1e77195cdc2c0b9b4aeea2f4d59f11f729f02af9920c4854ef72e"};
    // dettato's one checkpoint, which holds its four networks.
    static constexpr ModelManifestEntry DETTATO {"dettato-f16.gguf",
                                                 73685536,
                                                 "73f08bfcad22c401c1e584229f2b49cb8097a9b539cc81fd29dca573d7585480",
                                                 "kobimusic/dettato-gguf",
                                                 "73f622a5173c8c9112d7cf2081a0402d73746fc7",
                                                 ""};

    switch (inModelSize) {
        case ModelSize::Dettato:
            return DETTATO;
        case ModelSize::Small:
            return SMALL;
        case ModelSize::Medium:
            return MEDIUM;
        case ModelSize::Large:
            return LARGE;
    }

    return MEDIUM;
}

/** @return The URL inEntry is downloaded from. */
inline std::string getModelUrl(const ModelManifestEntry& inEntry)
{
    const std::string directory = inEntry.directory;

    return "https://huggingface.co/" + std::string(inEntry.repo) + "/resolve/" + inEntry.revision + "/"
           + (directory.empty() ? directory : directory + "/") + inEntry.fileName;
}

#endif // ModelManifest_h
