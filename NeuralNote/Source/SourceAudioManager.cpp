//
// Created by Damien Ronssin on 19.06.23.
//

#include <memory>

#include "SourceAudioManager.h"
#include "PluginProcessor.h"

SourceAudioManager::SourceAudioManager(NeuralNoteAudioProcessor* inProcessor)
    : mProcessor(inProcessor)
{
}

SourceAudioManager::~SourceAudioManager()
{
    for (auto& file: mFilesToDelete) {
        file.deleteFile();
    }
}

void SourceAudioManager::prepareToPlay(double inSampleRate, int inSamplesPerBlock)
{
    if (mIsRecording.load()) {
        stopRecording();
    }

    mSampleRate = inSampleRate;
    mInternalMonoBuffer.setSize(1, inSamplesPerBlock);
    mDownSampler.prepareToPlay(inSampleRate, inSamplesPerBlock, BASIC_PITCH_SAMPLE_RATE);
    mInternalDownsampledBuffer.setSize(1,
                                       (int) std::ceil(BASIC_PITCH_SAMPLE_RATE / inSampleRate * inSamplesPerBlock) + 5);

    if (mProcessor->getState() == PopulatedAudioAndMidiRegions && mSampleRate != mSourceAudioSampleRate) {
        AudioBuffer<float> tmp_buffer;
        AudioUtils::resampleBuffer(mSourceAudio, tmp_buffer, mSourceAudioSampleRate, mSampleRate);
        mSourceAudio = tmp_buffer;
        mSourceAudioSampleRate = mSampleRate;
    }
}

void SourceAudioManager::processBlock(const AudioBuffer<float>& inBuffer)
{
    if (mIsRecording) {
        ScopedLock sl(mWriterLock);

        // TODO: make sure there are always two channels
        // Write incoming audio to file at native sample rate
        bool result = mThreadedWriter->write(inBuffer.getArrayOfReadPointers(), inBuffer.getNumSamples());
        jassertquiet(result);
        mNumSamplesAcquired += inBuffer.getNumSamples();
        mDuration = (double) mNumSamplesAcquiredDown / BASIC_PITCH_SAMPLE_RATE;

        // Downmix to mono
        mInternalMonoBuffer.copyFrom(0, 0, inBuffer, 0, 0, inBuffer.getNumSamples());
        for (int ch = 1; ch < inBuffer.getNumChannels(); ch++) {
            mInternalMonoBuffer.addFrom(0, 0, inBuffer, ch, 0, inBuffer.getNumSamples());
        }

        mInternalMonoBuffer.applyGain(1.0f / (float) inBuffer.getNumChannels());

        // Downsample to basic pitch sample rate
        int num_samples_down = mDownSampler.processBlock(mInternalMonoBuffer.getReadPointer(0),
                                                         mInternalDownsampledBuffer.getWritePointer(0),
                                                         inBuffer.getNumSamples());
        jassert(num_samples_down <= mInternalDownsampledBuffer.getNumSamples());

        // Write downsampled audio to file at downsampled sample rate
        bool result_down =
            mThreadedWriterDown->write(mInternalDownsampledBuffer.getArrayOfReadPointers(), num_samples_down);
        jassertquiet(result_down);

        mNumSamplesAcquiredDown += num_samples_down;
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
    File neural_note_dir =
        File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("NeuralNote");

    if (!neural_note_dir.isDirectory()) {
        neural_note_dir.createDirectory();
    }

    mRecordedFile = neural_note_dir.getChildFile("recorded_audio.wav");
    mRecordedFileDown = neural_note_dir.getChildFile("recorded_audio_downsampled.wav");

    size_t i = 1;

    while (mRecordedFile.existsAsFile() || mRecordedFileDown.existsAsFile()) {
        mRecordedFile = neural_note_dir.getChildFile("recorded_audio_" + std::to_string(i) + ".wav");
        mRecordedFileDown = neural_note_dir.getChildFile("recorded_audio_downsampled_" + std::to_string(i) + ".wav");
    }

    Result file_creation_result = mRecordedFile.create();
    Result file_creation_result_down = mRecordedFileDown.create();

    if (!file_creation_result.wasOk() || !file_creation_result_down.wasOk()) {
        mProcessor->clear();
        NativeMessageBox::showMessageBoxAsync(
            juce::MessageBoxIconType::NoIcon, "Error", "File creation for recording failed.");
        return;
    }

    mFilesToDelete.push_back(mRecordedFile);
    mFilesToDelete.push_back(mRecordedFileDown);

    // Init first writer at native sample rate (stereo)
    juce::WavAudioFormat format;
    juce::StringPairArray meta_data_values;

    auto* wav_writer =
        format.createWriterFor(new juce::FileOutputStream(mRecordedFile), mSampleRate, 2, 16, meta_data_values, 0);

    mWriterThread.startThread();
    mThreadedWriter = std::make_unique<juce::AudioFormatWriter::ThreadedWriter>(wav_writer, mWriterThread, 32768);

    // Init second writer at basic pitch sample rate (mono)
    juce::WavAudioFormat format_down;
    juce::StringPairArray meta_data_values_down;

    auto* wav_writer_down = format_down.createWriterFor(
        new juce::FileOutputStream(mRecordedFileDown), BASIC_PITCH_SAMPLE_RATE, 1, 16, meta_data_values_down, 0);

    mWriterThreadDown.startThread();
    mThreadedWriterDown =
        std::make_unique<juce::AudioFormatWriter::ThreadedWriter>(wav_writer_down, mWriterThreadDown, 32768);
    mDownSampler.reset();

    mNumSamplesAcquired = 0;
    mNumSamplesAcquiredDown = 0;

    mIsRecording.store(true);
    mProcessor->setStateToRecording();
}

void SourceAudioManager::stopRecording()
{
    {
        ScopedLock sl(mWriterLock);
        mIsRecording.store(false);
    }

    mThreadedWriter.reset();
    mThreadedWriterDown.reset();

    mWriterThread.stopThread(1000);
    mWriterThreadDown.stopThread(1000);

    AudioUtils::loadAudioFile(mRecordedFile, mSourceAudio, mSourceAudioSampleRate);
    jassert(mSourceAudioSampleRate == mSampleRate);

    double dummy_sr;
    AudioUtils::loadAudioFile(mRecordedFileDown, mDownsampledSourceAudio, dummy_sr);
    jassert(dummy_sr == BASIC_PITCH_SAMPLE_RATE);

    mProcessor->setStateToProcessing();
    mProcessor->launchTranscribeJob();
}

bool SourceAudioManager::onFileDrop(const File& inFile)
{
    if (mProcessor->getState() == EmptyAudioAndMidiRegions || mProcessor->getState() == PopulatedAudioAndMidiRegions) {
        bool success = AudioUtils::loadAudioFile(inFile, mSourceAudio, mSourceAudioSampleRate);

        if (!success) {
            mProcessor->clear();
            juce::NativeMessageBox::showMessageBoxAsync(
                juce::MessageBoxIconType::NoIcon,
                "Could not load the audio sample.",
                "Check your file format (Accepted formats: .wav, .aiff, .flac).");
            return false;
        }

        // Downsample to basic pitch sample rate
        AudioUtils::resampleBuffer(
            mSourceAudio, mDownsampledSourceAudio, mSourceAudioSampleRate, BASIC_PITCH_SAMPLE_RATE);

        // Resample to current plugin sample rate for playback
        if (mSourceAudioSampleRate != mSampleRate) {
            AudioBuffer<float> tmp_buffer;
            AudioUtils::resampleBuffer(mSourceAudio, tmp_buffer, mSourceAudioSampleRate, mSampleRate);
            mSourceAudio = tmp_buffer;
            mSourceAudioSampleRate = mSampleRate;
        }

        mNumSamplesAcquiredDown = mDownsampledSourceAudio.getNumSamples();
        mNumSamplesAcquired = mSourceAudio.getNumSamples();
        mDuration = (double) mNumSamplesAcquiredDown / BASIC_PITCH_SAMPLE_RATE;

        mDroppedFilename = inFile.getFileName().toStdString();

        mProcessor->setStateToProcessing();
        mProcessor->launchTranscribeJob();
    } else {
        jassertfalse;
    }

    return true;
}

void SourceAudioManager::clear()
{
    // TODO:
}

AudioBuffer<float>& SourceAudioManager::getDownsampledSourceAudioForTranscription()
{
    return mDownsampledSourceAudio;
}

AudioBuffer<float>& SourceAudioManager::getSourceAudioForPlayback()
{
    return mSourceAudio;
}

std::string SourceAudioManager::getDroppedFilename() const
{
    return mDroppedFilename;
}

int SourceAudioManager::getNumSamplesDownAcquired() const
{
    return (int) mNumSamplesAcquiredDown;
}

double SourceAudioManager::getAudioSampleDuration() const
{
    return mDuration;
}
