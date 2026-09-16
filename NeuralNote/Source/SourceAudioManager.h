//
// Created by Damien Ronssin on 19.06.23.
//

#ifndef SourceAudioManager_h
#define SourceAudioManager_h

#include <JuceHeader.h>
#include "TranscriptionConstants.h"
#include "Resampler.h"
#include "AudioUtils.h"
#include "NNFileUtils.h"
#include "WaveformPeaks.h"

class NeuralNoteAudioProcessor;

/**
 * Broadcasts a change whenever the source audio grows or goes away, which is what the waveform
 * listens to in order to resize and redraw.
 */
class SourceAudioManager
    : public ValueTree::Listener
    , public juce::ChangeBroadcaster
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
     *
     * Always single channel: the model takes one signal, and it has to be the whole mix. Both the
     * recording and the file-drop path average the input channels while resampling.
     *
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
     * Seconds to add to every note time on export so that time zero of the MIDI file is a bar
     * line, letting the file be dropped on a bar in the host and land where it was played.
     *
     * Zero unless the take was recorded against a running transport. Message thread.
     */
    double getExportStartOffsetSeconds() const { return mExportStartOffsetSeconds.load(); }

    /**
     * Min/max peaks over the downsampled source audio, for drawing the waveform.
     * Read through WaveformPeaks::Reader: it is appended to from the writer thread while recording.
     */
    const WaveformPeaks& getWaveformPeaks() const;

private:
    /**
     * Takes the downsampled audio straight off the writer's FIFO while recording, so the peaks grow
     * with the take. The file being written alongside is not read back until recording stops.
     */
    class PeaksReceiver : public AudioFormatWriter::ThreadedWriter::IncomingDataReceiver
    {
    public:
        explicit PeaksReceiver(SourceAudioManager& inOwner)
            : mOwner(inOwner)
        {
        }

        void reset(int numChannels, double sampleRate, int64 totalSamplesInSource) override;

        void addBlock(int64 sampleNumberInSource,
                      const AudioBuffer<float>& newData,
                      int startOffsetInBuffer,
                      int numSamples) override;

    private:
        SourceAudioManager& mOwner;
    };

    void valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property) override;

    /**
     * Reads the host timeline once per take, the first time the transport is seen rolling, and
     * reduces it to the export offset and the tempo. Audio thread, while recording.
     */
    void _tryCaptureHostTimeline();

    void _deleteFilesToDelete();

    NeuralNoteAudioProcessor* mProcessor;

    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> mThreadedWriter;
    juce::TimeSliceThread mWriterThread = juce::TimeSliceThread("Source Audio Writer Thread");

    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> mThreadedWriterDown;
    juce::TimeSliceThread mWriterThreadDown = juce::TimeSliceThread("Downsampled Source Audio Writer Thread");
    CriticalSection mWriterLock;

    Resampler mDownSampler = {};

    WaveformPeaks mWaveformPeaks;
    PeaksReceiver mPeaksReceiver {*this};

    const File mRecordingsDir = NNFileUtils::getRecordingsDirectory();
    File mSourceFile;
    File mRecordedFileDown;

    AudioBuffer<float> mSourceAudio;
    AudioBuffer<float> mDownsampledSourceAudio; // Always at basic pitch sample rate

    // Sample rate for mSourceAudio buffer
    double mSourceAudioSampleRate = 44100;

    std::vector<juce::File> mFilesToDelete;

    double mSampleRate = 44100;

    // Captured on the audio thread while recording, read on the message thread at export. Both
    // stay zero for a take made with the transport stopped, and for a dropped file.
    std::atomic<double> mExportStartOffsetSeconds = 0.0;
    std::atomic<double> mHostBpm = 0.0;

    // Audio thread only, reset by startRecording before mIsRecording goes true.
    bool mCapturedHostTimeline = false;
    int mNumPlayingBlocks = 0;

    // Written on the audio thread while recording, read on the message thread to size the
    // waveform and drive the time displays.
    std::atomic<unsigned long long> mNumSamplesAcquired = 0;
    std::atomic<unsigned long long> mNumSamplesAcquiredDown = 0;
    std::atomic<double> mDuration = 0.0;

    String mDroppedFilename;

    AudioBuffer<float> mInternalDownsampledBuffer;

    std::atomic<bool> mIsRecording = false;
};

#endif // SourceAudioManager_h
