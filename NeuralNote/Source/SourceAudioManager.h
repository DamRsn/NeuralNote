//
// Created by Damien Ronssin on 19.06.23.
//

#ifndef SourceAudioManager_h
#define SourceAudioManager_h

#include <JuceHeader.h>
#include "BasicPitchConstants.h"
#include "DownSampler.h"

class SourceAudioManager
{
public:
    SourceAudioManager();

    void prepareToPlay(double inSampleRate, int inSamplesPerBlock);

    void processBlock(const AudioBuffer<float>& inBuffer);

    void startRecording();

    void stopRecording();

    bool isRecording() const;

private:
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> mThreadedWriter;
    juce::TimeSliceThread mWriterThread = juce::TimeSliceThread("Source Audio Writer Thread");

    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> mThreadedWriterDown;
    juce::TimeSliceThread mWriterThreadDown = juce::TimeSliceThread("Downsampled Source Audio Writer Thread");
    DownSampler mDownSampler;

    CriticalSection mWriterLock;

    juce::File mFile;
    juce::File mFileDown;

    double mSampleRate = 44100;
    AudioBuffer<float> mInternalMonoBuffer;
    AudioBuffer<float> mInternalDownsampledBuffer;

    std::atomic<bool> mIsRecording = false;
};

#endif // SourceAudioManager_h
