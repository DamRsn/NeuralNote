//
// Created by Damien Ronssin on 12.09.26.
//

#ifndef ModelDownloader_h
#define ModelDownloader_h

#include <array>
#include <memory>

#include <JuceHeader.h>

#include "TranscriptionConstants.h"

/**
 * Downloads checkpoints into the models directory, one background thread per size.
 *
 * Meant to be one per process, through juce::SharedResourcePointer, so every instance in a host
 * shows the same download. Another process downloading the same size is kept out by an
 * InterProcessLock.
 *
 * A download is written to NNFileUtils::getModelPartFile and resumes from it with a Range request,
 * so a dropped connection or a closed plugin only costs what was not written yet. The finished
 * part is checked against the digest in ModelManifest.h, then renamed into place.
 *
 * start, cancel and getStatus are for the message thread.
 */
class ModelDownloader
{
public:
    enum class Phase { Idle, Downloading, Verifying, Failed };

    struct Status {
        Phase phase = Phase::Idle;

        // Outside Downloading, what the part file holds: where the next start resumes from.
        juce::int64 downloadedBytes = 0;
        juce::int64 totalBytes = 0;

        // Only set in Failed. Short enough to show as is.
        juce::String errorMessage;

        bool isBusy() const { return phase == Phase::Downloading || phase == Phase::Verifying; }

        /** In [0, 1]. */
        float getProgress() const;
    };

    ModelDownloader();

    /** Stops every download, keeping their part files. Blocks while one is being verified. */
    ~ModelDownloader();

    /** Does nothing while this size is already busy. Resumes a part file if there is one. */
    void start(ModelSize inModelSize);

    /** Returns without waiting. The part file is kept, so the next start resumes. */
    void cancel(ModelSize inModelSize);

    Status getStatus(ModelSize inModelSize) const;

private:
    class Job;

    std::array<std::unique_ptr<Job>, ALL_MODEL_SIZES.size()> mJobs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModelDownloader)
};

#endif // ModelDownloader_h
