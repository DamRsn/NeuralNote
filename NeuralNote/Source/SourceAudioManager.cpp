//
// Created by Damien Ronssin on 19.06.23.
//

#include <memory>

#include "SourceAudioManager.h"
#include "PluginProcessor.h"

SourceAudioManager::SourceAudioManager(NeuralNoteAudioProcessor* inProcessor)
    : mProcessor(inProcessor)
{
    mProcessor->addListenerToStateValueTree(this);
    jassert(mProcessor->getValueTree().hasProperty(NnId::SourceAudioNativeSrPathId));
}

SourceAudioManager::~SourceAudioManager()
{
    mProcessor->removeListenerFromStateValueTree(this);
}

void SourceAudioManager::prepareToPlay(double inSampleRate, int inSamplesPerBlock)
{
    if (mIsRecording.load()) {
        stopRecording();
    }

    mSampleRate = inSampleRate;
    mDownSampler.prepareToPlay(inSampleRate, inSamplesPerBlock, TRANSCRIPTION_SAMPLE_RATE);
    mInternalDownsampledBuffer.setSize(
        1, static_cast<int>(std::ceil(TRANSCRIPTION_SAMPLE_RATE / inSampleRate * inSamplesPerBlock)) + 5);

    // canPlay(), not a list of states: the question is whether there is source audio to keep
    // playable, which is true from the moment a file is loaded and before anything is transcribed.
    if (mProcessor->canPlay() && !juce::exactlyEqual(mSampleRate, mSourceAudioSampleRate)) {
        AudioBuffer<float> tmp_buffer;
        AudioUtils::resampleBuffer(mSourceAudio, tmp_buffer, mSourceAudioSampleRate, mSampleRate);
        mSourceAudio = std::move(tmp_buffer);
        mSourceAudioSampleRate = mSampleRate;
    }
}

namespace
{
// Logic reports a stale playhead position for the first blocks after the transport starts.
constexpr int NUM_SETTLING_BLOCKS = 2;
} // namespace

void SourceAudioManager::_tryCaptureHostTimeline()
{
    auto* playhead = mProcessor->getPlayHead();

    if (playhead == nullptr) {
        return;
    }

    const auto position = playhead->getPosition();

    if (!position.hasValue() || !position->getIsPlaying()) {
        return;
    }

    if (mNumPlayingBlocks++ < NUM_SETTLING_BLOCKS) {
        return;
    }

    const auto bpm = position->getBpm();

    if (!bpm.hasValue() || *bpm <= 0.0) {
        return;
    }

    const auto time_signature = position->getTimeSignature();
    const int numerator = time_signature.hasValue() ? time_signature->numerator : 4;
    const int denominator = time_signature.hasValue() ? time_signature->denominator : 4;

    const auto ppq = position->getPpqPosition();
    const auto ppq_last_bar = position->getPpqPositionOfLastBarStart();

    double offset_seconds = 0.0;

    if (ppq.hasValue() && ppq_last_bar.hasValue() && numerator > 0 && denominator > 0) {
        const double bar_seconds = (numerator * 4.0 / denominator) * 60.0 / *bpm;
        const double seconds_into_take = static_cast<double>(mNumSamplesAcquired.load()) / mSampleRate;

        // Where this bar started, in take time. Positive if the transport started after the take
        // did, so step back whole bars until the line is at or before the take's own zero.
        const double bar_start = seconds_into_take - (*ppq - *ppq_last_bar) * 60.0 / *bpm;

        offset_seconds = -(bar_start - std::ceil(bar_start / bar_seconds) * bar_seconds);
    }

    mExportStartOffsetSeconds.store(offset_seconds);
    mHostBpm.store(*bpm);
    mCapturedHostTimeline = true;
}

void SourceAudioManager::processBlock(const AudioBuffer<float>& inBuffer)
{
    if (mIsRecording) {
        ScopedLock sl(mWriterLock);

        if (!mCapturedHostTimeline) {
            // Before the count moves on, so the reference is where the take stands entering here.
            _tryCaptureHostTimeline();
        }

        // Write incoming audio to file at native sample rate
        bool result = mThreadedWriter->write(inBuffer.getArrayOfReadPointers(), inBuffer.getNumSamples());
        jassertquiet(result);
        mNumSamplesAcquired += static_cast<unsigned long long>(inBuffer.getNumSamples());

        // Downmix to mono and downsample to the rate the model expects, in one pass
        int num_samples_down = mDownSampler.processBlock(inBuffer.getArrayOfReadPointers(),
                                                         inBuffer.getNumChannels(),
                                                         mInternalDownsampledBuffer.getWritePointer(0),
                                                         inBuffer.getNumSamples());
        jassert(num_samples_down <= mInternalDownsampledBuffer.getNumSamples());

        // Write downsampled audio to file at downsampled sample rate
        bool result_down =
            mThreadedWriterDown->write(mInternalDownsampledBuffer.getArrayOfReadPointers(), num_samples_down);
        jassertquiet(result_down);

        mNumSamplesAcquiredDown += static_cast<unsigned long long>(num_samples_down);
        mDuration = static_cast<double>(mNumSamplesAcquiredDown) / TRANSCRIPTION_SAMPLE_RATE;
    }
}

void SourceAudioManager::startRecording()
{
    // If already recording (should not happen but if it does, simply return)
    if (mIsRecording.load()) {
        jassertfalse;
        return;
    }

    // Prepare files to be written
    bool dir_ready = NNFileUtils::ensureDirectoryExists(mRecordingsDir);
    jassertquiet(dir_ready);

    mDroppedFilename = "";

    const String prefix = NNFileUtils::RECORDING_FILENAME_PREFIX;
    String timestamp = Time::getCurrentTime().formatted("%Y-%m-%d_%H-%M-%S");

    mSourceFile = mRecordingsDir.getChildFile(prefix + timestamp + ".wav");
    mRecordedFileDown = mRecordingsDir.getChildFile(prefix + timestamp + "_downsampled.wav");

    size_t i = 1;

    while (mSourceFile.existsAsFile() || mRecordedFileDown.existsAsFile()) {
        mSourceFile = mRecordingsDir.getChildFile(prefix + timestamp + "_" + String(i) + ".wav");
        mRecordedFileDown = mRecordingsDir.getChildFile(prefix + timestamp + "_" + String(i) + "_downsampled.wav");
        i += 1;
    }

    Result file_creation_result = mSourceFile.create();
    Result file_creation_result_down = mRecordedFileDown.create();

    if (!file_creation_result.wasOk() || !file_creation_result_down.wasOk()) {
        mProcessor->clear();
        NativeMessageBox::showMessageBoxAsync(
            MessageBoxIconType::NoIcon, "Error", "File creation for recording failed.");
        return;
    }

    mFilesToDelete.push_back(mSourceFile);
    mFilesToDelete.push_back(mRecordedFileDown);

    // Init first writer at native sample rate (stereo)
    WavAudioFormat format;

    std::unique_ptr<OutputStream> stream = std::make_unique<FileOutputStream>(mSourceFile);
    auto wav_writer = format.createWriterFor(stream,
                                             AudioFormatWriterOptions()
                                                 .withSampleRate(mSampleRate)
                                                 .withNumChannels(std::min(mProcessor->getTotalNumInputChannels(), 2))
                                                 .withBitsPerSample(16));

    mWriterThread.startThread();
    mThreadedWriter = std::make_unique<AudioFormatWriter::ThreadedWriter>(wav_writer.release(), mWriterThread, 32768);

    // Init second writer at basic pitch sample rate (mono)
    WavAudioFormat format_down;

    std::unique_ptr<OutputStream> stream_down = std::make_unique<FileOutputStream>(mRecordedFileDown);
    auto wav_writer_down = format_down.createWriterFor(
        stream_down,
        AudioFormatWriterOptions().withSampleRate(TRANSCRIPTION_SAMPLE_RATE).withNumChannels(1).withBitsPerSample(16));

    mWriterThreadDown.startThread();
    mThreadedWriterDown =
        std::make_unique<AudioFormatWriter::ThreadedWriter>(wav_writer_down.release(), mWriterThreadDown, 32768);
    mDownSampler.reset();

    // setDataReceiver resets the receiver for us, which is what clears the peaks from the last take.
    mThreadedWriterDown->setDataReceiver(&mPeaksReceiver);

    mNumSamplesAcquired = 0;
    mNumSamplesAcquiredDown = 0;

    mExportStartOffsetSeconds = 0.0;
    mHostBpm = 0.0;
    mCapturedHostTimeline = false;
    mNumPlayingBlocks = 0;

    mIsRecording.store(true);
    mProcessor->setStateToRecording();
}

void SourceAudioManager::stopRecording()
{
    if (!mIsRecording.load()) {
        return;
    }

    {
        ScopedLock sl(mWriterLock);
        mIsRecording.store(false);
    }

    mThreadedWriter.reset();
    mThreadedWriterDown.reset();

    mWriterThread.stopThread(1000);
    mWriterThreadDown.stopThread(1000);

    // Record then stop before a block came in: there is no audio to load, and going to AudioLoaded
    // would offer transport and transcription over an empty buffer.
    if (mNumSamplesAcquired == 0 || mNumSamplesAcquiredDown == 0) {
        mProcessor->clear();
        return;
    }

    bool success = AudioUtils::loadAudioFile(mSourceFile, mSourceAudio, mSourceAudioSampleRate);
    jassert(juce::exactlyEqual(mSourceAudioSampleRate, mSampleRate));

    // Should def not happen
    if (!success) {
        mProcessor->clear();
        NativeMessageBox::showMessageBoxAsync(
            MessageBoxIconType::NoIcon, "Could not load the recorded audio sample.", "");
        return;
    }

    double dummy_sr;
    success = AudioUtils::loadAudioFile(mRecordedFileDown, mDownsampledSourceAudio, dummy_sr);
    jassert(juce::exactlyEqual(dummy_sr, TRANSCRIPTION_SAMPLE_RATE));

    // Should def not happen
    if (!success) {
        mProcessor->clear();
        NativeMessageBox::showMessageBoxAsync(
            MessageBoxIconType::NoIcon, "Could not load the recorded audio sample.", "");
        return;
    }

    // Rebuild from the buffer that will actually be played and transcribed, rather than keeping the
    // peaks accumulated live off the FIFO: this went to disk as 16-bit and came back, so the two
    // are no longer quite the same audio.
    // The tempo the take was played to is the one to export it at, so the toolbar field follows it.
    if (const double host_bpm = mHostBpm.load(); host_bpm > 0.0) {
        mProcessor->getValueTree().setProperty(NnId::ExportTempoId, host_bpm, nullptr);
    }

    mWaveformPeaks.buildFrom(
        {mDownsampledSourceAudio.getReadPointer(0), static_cast<size_t>(mDownsampledSourceAudio.getNumSamples())});
    sendChangeMessage();

    auto& tree = mProcessor->getValueTree();
    tree.setPropertyExcludingListener(this, NnId::SourceAudioNativeSrPathId, mSourceFile.getFullPathName(), nullptr);

    mProcessor->setStateToAudioLoaded();
}

bool SourceAudioManager::onFileDrop(const File& inFile)
{
    const State state = mProcessor->getState();

    if (state == EmptyAudioAndMidiRegions || state == AudioLoaded || state == PopulatedAudioAndMidiRegions) {
        mProcessor->clear();
        bool success = AudioUtils::loadAudioFile(inFile, mSourceAudio, mSourceAudioSampleRate);

        if (!success) {
            mProcessor->clear();
            NativeMessageBox::showMessageBoxAsync(
                MessageBoxIconType::NoIcon,
                "Could not load the audio file.",
                "Check your file format (Accepted formats: .wav, .aiff, .flac, .mp3, .ogg).");
            return false;
        }

        // Downmix and downsample to the rate the model expects, in one pass
        AudioUtils::resampleBufferToMono(
            mSourceAudio, mDownsampledSourceAudio, mSourceAudioSampleRate, TRANSCRIPTION_SAMPLE_RATE);

        // Resample to current plugin sample rate for playback
        if (!juce::exactlyEqual(mSourceAudioSampleRate, mSampleRate)) {
            AudioBuffer<float> tmp_buffer;
            AudioUtils::resampleBuffer(mSourceAudio, tmp_buffer, mSourceAudioSampleRate, mSampleRate);
            mSourceAudio = std::move(tmp_buffer);
            mSourceAudioSampleRate = mSampleRate;
        }

        mNumSamplesAcquiredDown = static_cast<unsigned long long>(mDownsampledSourceAudio.getNumSamples());
        mNumSamplesAcquired = static_cast<unsigned long long>(mSourceAudio.getNumSamples());
        mDuration = static_cast<double>(mNumSamplesAcquiredDown) / TRANSCRIPTION_SAMPLE_RATE;

        mDroppedFilename = inFile.getFileNameWithoutExtension();
        mSourceFile = inFile;

        mExportStartOffsetSeconds = 0.0;
        mHostBpm = 0.0;

        auto& tree = mProcessor->getValueTree();
        tree.setPropertyExcludingListener(this, NnId::SourceAudioNativeSrPathId, inFile.getFullPathName(), nullptr);

        mWaveformPeaks.buildFrom(
            {mDownsampledSourceAudio.getReadPointer(0), static_cast<size_t>(mDownsampledSourceAudio.getNumSamples())});
        sendChangeMessage();

        // No transcription yet: the user starts one from the piano roll, once they have had the
        // chance to pick which instruments it should look for.
        mProcessor->setStateToAudioLoaded();

    } else {
        jassertfalse;
    }

    return true;
}

void SourceAudioManager::clear()
{
    if (mIsRecording) {
        stopRecording();
    }

    // Before the buffers go: the peaks borrow mDownsampledSourceAudio rather than copying it.
    mWaveformPeaks.clear();

    mSourceAudio = {};
    mDownsampledSourceAudio = {};

    mNumSamplesAcquiredDown = 0;
    mNumSamplesAcquired = 0;
    mDuration = 0.0;

    mExportStartOffsetSeconds = 0.0;
    mHostBpm = 0.0;

    mDownSampler.reset();

    _deleteFilesToDelete();

    mProcessor->getValueTree().setPropertyExcludingListener(this, NnId::SourceAudioNativeSrPathId, String(), nullptr);

    mSourceFile = File();
    mRecordedFileDown = File();
    mDroppedFilename = "";

    sendChangeMessage();
}

void SourceAudioManager::PeaksReceiver::reset(int numChannels, double sampleRate, int64 totalSamplesInSource)
{
    ignoreUnused(numChannels, sampleRate, totalSamplesInSource);

    mOwner.mWaveformPeaks.clear();
}

void SourceAudioManager::PeaksReceiver::addBlock(int64 sampleNumberInSource,
                                                 const AudioBuffer<float>& newData,
                                                 int startOffsetInBuffer,
                                                 int numSamples)
{
    ignoreUnused(sampleNumberInSource);

    // Mono: this is the writer that runs at the transcription rate.
    mOwner.mWaveformPeaks.append({newData.getReadPointer(0, startOffsetInBuffer), static_cast<size_t>(numSamples)});

    mOwner.sendChangeMessage();
}

AudioBuffer<float>& SourceAudioManager::getDownsampledSourceAudioForTranscription()
{
    // The caller reads channel 0 and nothing else, so a second channel here would be half the mix
    // silently thrown away rather than a harmless extra.
    jassert(mDownsampledSourceAudio.getNumChannels() <= 1);

    return mDownsampledSourceAudio;
}

AudioBuffer<float>& SourceAudioManager::getSourceAudioForPlayback()
{
    return mSourceAudio;
}

String SourceAudioManager::getDroppedFilename() const
{
    return mDroppedFilename;
}

int SourceAudioManager::getNumSamplesDownAcquired() const
{
    return static_cast<int>(mNumSamplesAcquiredDown);
}

double SourceAudioManager::getAudioSampleDuration() const
{
    return mDuration;
}

const WaveformPeaks& SourceAudioManager::getWaveformPeaks() const
{
    return mWaveformPeaks;
}

void SourceAudioManager::valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property)
{
    if (property == NnId::SourceAudioNativeSrPathId) {
        auto path = treeWhosePropertyHasChanged.getProperty(property).toString();

        if (path != mSourceFile.getFullPathName()) {
            if (path.isEmpty()) {
                clear();
                return;
            }

            onFileDrop(File(path));

            // If loading state from recorded audio, add to files to delete on clear.
            if (NNFileUtils::isRecordingFile(mSourceFile)) {
                mRecordedFileDown =
                    mRecordingsDir.getChildFile(mSourceFile.getFileNameWithoutExtension() + "_downsampled.wav");
                mFilesToDelete.push_back(mSourceFile);
                mFilesToDelete.push_back(mRecordedFileDown);
                mDroppedFilename = "";
            }
        }
    }
}

void SourceAudioManager::_deleteFilesToDelete()
{
    for (auto& file: mFilesToDelete) {
        // Make sure we only ever delete files that have been recorded, not loaded from disk.
        if (NNFileUtils::isRecordingFile(file)) {
            bool res = file.deleteFile();
            jassertquiet(res);
        } else {
            jassertfalse;
        }
    }

    mFilesToDelete.clear();
}
