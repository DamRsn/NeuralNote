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

class SourceAudioManager
{
public:
    SourceAudioManager(NeuralNoteAudioProcessor* inProcessor);

    ~SourceAudioManager();

    void prepareToPlay(double inSampleRate, int inSamplesPerBlock);

    void processBlock(const AudioBuffer<float>& inBuffer);

    void startRecording();

    void stopRecording();

    bool onFileDrop(const File& inFile);

    // TODO: clear and reset everything in this class.
    void clear();

    AudioBuffer<float>& getDownsampledSourceAudioForTranscription();

    AudioBuffer<float>& getSourceAudioForPlayback();

    std::string getDroppedFilename() const;

    int getNumSamplesDownAcquired() const;

    /* Returns the duration in seconds of the audio acquired for transcription */
    double getAudioSampleDuration() const;

private:
    NeuralNoteAudioProcessor* mProcessor;

    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> mThreadedWriter;
    juce::TimeSliceThread mWriterThread = juce::TimeSliceThread("Source Audio Writer Thread");

    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> mThreadedWriterDown;
    juce::TimeSliceThread mWriterThreadDown = juce::TimeSliceThread("Downsampled Source Audio Writer Thread");
    CriticalSection mWriterLock;

    Resampler mDownSampler;

    File mRecordedFile;
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

    std::string mDroppedFilename;

    AudioBuffer<float> mInternalMonoBuffer;
    AudioBuffer<float> mInternalDownsampledBuffer;

    std::atomic<bool> mIsRecording = false;
};

#endif // SourceAudioManager_h
