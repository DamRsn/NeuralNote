//
// Created by Damien Ronssin on 19.06.23.
//

#ifndef SourceAudioManager_h
#define SourceAudioManager_h

#include <JuceHeader.h>
#include "BasicPitchConstants.h"
#include "Resampler.h"
#include "AudioUtils.h"

class NeuralNoteAudioProcessor;

class SourceAudioManager : public ValueTree::Listener
{
public:
    explicit SourceAudioManager(NeuralNoteAudioProcessor* inProcessor);

    ~SourceAudioManager() override;

    /**
     * PrepareToPlay
     * @param inSampleRate Audio sample rate
     * @param inSamplesPerBlock Number of samples per block
     */
    void prepareToPlay(double inSampleRate, int inSamplesPerBlock);

    /**
     * Function to call in NeuralNote audio processor. Will handle recording if needed.
     * @param inBuffer Input audio buffer
     */
    void processBlock(const AudioBuffer<float>& inBuffer);

    /**
     * Function to call when start record button is clicked.
     * Will prepare everything needed to record and the recording will start in the next processBlock
     */
    void startRecording();

    /**
     * Function to call when stop record button is clicked.
     * Will stop properly the recording and then launch the transcription.
     */
    void stopRecording();

    /**
     * Function to call when a file is dropped on the audio region to load it.
     * @param inFile Audio file to load
     * @return Whether audio file load was successful
     */
    bool onFileDrop(const File& inFile);

    /**
     * Stop recording if needed and then reset/clear everything owned by this class.
     */
    void clear();

    /**
     * To call only when the recording/file loading is fully completed, otherwise you'll get and empty buffer.
     * @return A reference to the downsampled source audio.
     */
    AudioBuffer<float>& getDownsampledSourceAudioForTranscription();

    /**
     * Get source audio at current processor sample rate.
     * @return Reference to source audio buffer (recorded or loaded from file).
     */
    AudioBuffer<float>& getSourceAudioForPlayback();

    /**
     * Return a string containing the filename of the dropped audio file.
     * If the source audio was recorded (not loaded from file), an empty string is returned.
     * @return Filename of dropped audio file, or empty string if source audio recorded.
     */
    String getDroppedFilename() const;

    /**
     * Get number of samples currently acquired (either recorded or loaded from file) at basic pitch sample rate (22.05 kHz)
     * Note that if recording is ongoing, those sample are not yet available in buffers returned by getDownsampledSourceAudioForTranscription() and getSourceAudioForPlayback().
     * @return Number of source audio samples already recorded or loaded at basic pitch sample rate.
     */
    int getNumSamplesDownAcquired() const;

    /**
     * Same as getNumSamplesDownAcquired() but in seconds instead of number of samples.
     * @return The duration in seconds of the audio acquired for transcription.
     */
    double getAudioSampleDuration() const;

    /**
     * @return Pointer to source audio thumbnail
     */
    AudioThumbnail* getAudioThumbnail();

private:
    void valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property) override;

    void _deleteFilesToDelete();

    NeuralNoteAudioProcessor* mProcessor;

    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> mThreadedWriter;
    juce::TimeSliceThread mWriterThread = juce::TimeSliceThread("Source Audio Writer Thread");

    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> mThreadedWriterDown;
    juce::TimeSliceThread mWriterThreadDown = juce::TimeSliceThread("Downsampled Source Audio Writer Thread");
    CriticalSection mWriterLock;

    Resampler mDownSampler = {};

    const int mSourceSamplesPerThumbnailSample = 128;
    juce::AudioFormatManager mThumbnailFormatManager;
    juce::AudioThumbnailCache mThumbnailCache;
    juce::AudioThumbnail mThumbnail;

    const File mNeuralNoteDir =
        File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("NeuralNote");
    File mSourceFile;
    File mRecordedFileDown;

    AudioBuffer<float> mSourceAudio;
    AudioBuffer<float> mDownsampledSourceAudio; // Always at basic pitch sample rate

    // Sample rate for mSourceAudio buffer
    double mSourceAudioSampleRate = 44100;

    std::vector<juce::File> mFilesToDelete;

    double mSampleRate = 44100;

    unsigned long long mNumSamplesAcquired = 0;
    unsigned long long mNumSamplesAcquiredDown = 0;
    double mDuration = 0.0;

    String mDroppedFilename;

    AudioBuffer<float> mInternalMonoBuffer;
    AudioBuffer<float> mInternalDownsampledBuffer;

    std::atomic<bool> mIsRecording = false;
};

#endif // SourceAudioManager_h
