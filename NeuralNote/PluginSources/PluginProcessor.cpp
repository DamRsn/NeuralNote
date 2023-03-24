#include "PluginProcessor.h"
#include "PluginEditor.h"

NeuralNoteAudioProcessor::NeuralNoteAudioProcessor()
    : mTree(*this, nullptr, "PARAMETERS", createParameterLayout())
    , mThreadPool(1)
{
    mAudioBufferForMIDITranscription.setSize(1, mMaxNumSamplesToConvert);
    mAudioBufferForMIDITranscription.clear();

    mJobLambda = [this]() { _runModel(); };
}

AudioProcessorValueTreeState::ParameterLayout
    NeuralNoteAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto mute = std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID {"MUTE", 1}, "Mute", true);
    params.push_back(std::move(mute));

    return {params.begin(), params.end()};
}

void NeuralNoteAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    mDownSampler.prepareToPlay(sampleRate, samplesPerBlock);

    mMonoBuffer.setSize(1, samplesPerBlock);
}

void NeuralNoteAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                            juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    const int num_in_channels = getTotalNumInputChannels();

    // Get tempo and time signature for UI.
    auto playhead_info = getPlayHead()->getPosition();
    if (playhead_info.hasValue())
    {
        if (playhead_info->getBpm().hasValue())
            mCurrentTempo = *playhead_info->getBpm();
        if (playhead_info->getTimeSignature().hasValue())
        {
            mCurrentTimeSignatureNum = playhead_info->getTimeSignature()->numerator;
            mCurrentTimeSignatureDenom = playhead_info->getTimeSignature()->denominator;
        }
    }

    if (mState.load() == Recording)
    {
        if (!mWasRecording)
        {
            mDownSampler.reset();
            mWasRecording = true;
            mPlayheadInfoStartRecord = getPlayHead()->getPosition();

            if (mPlayheadInfoStartRecord.hasValue())
            {
                mIsPlayheadPlaying = mPlayheadInfoStartRecord->getIsPlaying();
            }
            else
            {
                mIsPlayheadPlaying = false;
            }

            mRhythmOptions.setInfo(false, mPlayheadInfoStartRecord);
        }

        // If we have reached maximum number of samples that can be processed: stop record and launch processing
        int num_new_down_samples =
            mDownSampler.numOutSamplesOnNextProcessBlock(buffer.getNumSamples());

        // If we reach the maximum number of sample that can be gathered,
        // or the playhead has stopped playing if it was at the start of the recording: stop recording.
        if (mNumSamplesAcquired + num_new_down_samples >= mMaxNumSamplesToConvert
            || (mIsPlayheadPlaying && !getPlayHead()->getPosition()->getIsPlaying()))
        {
            mWasRecording = false;
            setStateToProcessing();
            launchTranscribeJob();
        }
        else
        {
            mMonoBuffer.copyFrom(0, 0, buffer, 0, 0, buffer.getNumSamples());

            if (num_in_channels == 2)
            {
                // Down-mix to mono
                mMonoBuffer.addFrom(0, 0, buffer, 1, 0, buffer.getNumSamples());
                buffer.applyGain(1.0f / static_cast<float>(num_in_channels));
            }

            // Fill buffer with 22050 Hz audio
            int num_samples_written = mDownSampler.processBlock(
                mMonoBuffer,
                mAudioBufferForMIDITranscription.getWritePointer(0, mNumSamplesAcquired),
                buffer.getNumSamples());

            jassert(num_samples_written <= num_new_down_samples);

            mNumSamplesAcquired += num_samples_written;
        }
    }
    else
    {
        // If we were previously recording but not anymore (user clicked record button to stop it).
        if (mWasRecording)
        {
            mWasRecording = false;
            launchTranscribeJob();
        }
    }

    auto isMute = mTree.getRawParameterValue("MUTE")->load() > 0.5;

    if (isMute)
        buffer.clear();
}

juce::AudioProcessorEditor* NeuralNoteAudioProcessor::createEditor()
{
    return new NeuralNoteEditor(*this);
}

void NeuralNoteAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
}

void NeuralNoteAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
}

void NeuralNoteAudioProcessor::clear()
{
    mNumSamplesAcquired = 0;
    mAudioBufferForMIDITranscription.clear();

    mPostProcessedNotes.clear();
    mPlayheadInfoStartRecord = juce::Optional<juce::AudioPlayHead::PositionInfo>();
    mDroppedFilename = "";

    mCurrentTempo = -1;
    mCurrentTimeSignatureNum = -1;
    mCurrentTimeSignatureDenom = -1;

    mMidiFileTempo = 120.0;

    mBasicPitch.reset();
    mWasRecording = false;
    mIsPlayheadPlaying = false;
    mState.store(EmptyAudioAndMidiRegions);
}

AudioBuffer<float>& NeuralNoteAudioProcessor::getAudioBufferForMidi()
{
    return mAudioBufferForMIDITranscription;
}

int NeuralNoteAudioProcessor::getNumSamplesAcquired() const
{
    return mNumSamplesAcquired;
}

void NeuralNoteAudioProcessor::setNumSamplesAcquired(int inNumSamplesAcquired)
{
    mNumSamplesAcquired = inNumSamplesAcquired;
}

void NeuralNoteAudioProcessor::launchTranscribeJob()
{
    jassert(mState.load() == Processing);
    if (mNumSamplesAcquired >= 1 * AUDIO_SAMPLE_RATE)
    {
        mThreadPool.addJob(mJobLambda);
    }
    else
    {
        clear();
    }
}

const std::vector<Notes::Event>& NeuralNoteAudioProcessor::getNoteEventVector() const
{
    return mPostProcessedNotes;
}

NeuralNoteAudioProcessor::Parameters* NeuralNoteAudioProcessor::getCustomParameters()
{
    return &mParameters;
}

const juce::Optional<juce::AudioPlayHead::PositionInfo>&
    NeuralNoteAudioProcessor::getPlayheadInfoOnRecordStart()
{
    return mPlayheadInfoStartRecord;
}

void NeuralNoteAudioProcessor::_runModel()
{
    mBasicPitch.setParameters(mParameters.noteSensibility,
                              mParameters.splitSensibility,
                              mParameters.minNoteDurationMs);

    mBasicPitch.transcribeToMIDI(mAudioBufferForMIDITranscription.getWritePointer(0),
                                 mNumSamplesAcquired);

    mNoteOptions.setParameters(NoteUtils::RootNote(mParameters.keyRootNote.load()),
                               NoteUtils::ScaleType(mParameters.keyType.load()),
                               NoteUtils::SnapMode(mParameters.keySnapMode.load()),
                               mParameters.minMidiNote.load(),
                               mParameters.maxMidiNote.load());

    auto post_processed_notes = mNoteOptions.process(mBasicPitch.getNoteEvents());

    mRhythmOptions.setParameters(
        RhythmUtils::TimeDivisions(mParameters.rhythmTimeDivision.load()),
        mParameters.rhythmQuantizationForce.load());

    mPostProcessedNotes = mRhythmOptions.quantize(post_processed_notes);

    Notes::dropOverlappingPitchBends(mPostProcessedNotes);
    Notes::mergeOverlappingNotesWithSamePitch(mPostProcessedNotes);

    mMidiFileTempo = mCurrentTempo.load() > 0 ? mCurrentTempo.load() : 120;

    mState.store(PopulatedAudioAndMidiRegions);
}

void NeuralNoteAudioProcessor::updateTranscription()
{
    jassert(mState == PopulatedAudioAndMidiRegions);

    if (mState == PopulatedAudioAndMidiRegions)
    {
        mBasicPitch.setParameters(mParameters.noteSensibility,
                                  mParameters.splitSensibility,
                                  mParameters.minNoteDurationMs);

        mBasicPitch.updateMIDI();
        updatePostProcessing();
    }
}

void NeuralNoteAudioProcessor::updatePostProcessing()
{
    jassert(mState == PopulatedAudioAndMidiRegions);

    if (mState == PopulatedAudioAndMidiRegions)
    {
        mNoteOptions.setParameters(NoteUtils::RootNote(mParameters.keyRootNote.load()),
                                   NoteUtils::ScaleType(mParameters.keyType.load()),
                                   NoteUtils::SnapMode(mParameters.keySnapMode.load()),
                                   mParameters.minMidiNote.load(),
                                   mParameters.maxMidiNote.load());

        auto post_processed_notes = mNoteOptions.process(mBasicPitch.getNoteEvents());

        mRhythmOptions.setParameters(
            RhythmUtils::TimeDivisions(mParameters.rhythmTimeDivision.load()),
            mParameters.rhythmQuantizationForce.load());

        mPostProcessedNotes = mRhythmOptions.quantize(post_processed_notes);

        Notes::dropOverlappingPitchBends(mPostProcessedNotes);
        Notes::mergeOverlappingNotesWithSamePitch(mPostProcessedNotes);
    }
}

void NeuralNoteAudioProcessor::setFileDrop(const std::string& inFilename)
{
    mRhythmOptions.setInfo(true);
    mDroppedFilename = inFilename;
}

std::string NeuralNoteAudioProcessor::getDroppedFilename() const
{
    return mDroppedFilename;
}

bool NeuralNoteAudioProcessor::canQuantize() const
{
    return mRhythmOptions.canPerformQuantization();
}

std::string NeuralNoteAudioProcessor::getTempoStr() const
{
    if (mPlayheadInfoStartRecord.hasValue()
        && mPlayheadInfoStartRecord->getBpm().hasValue())
        return std::to_string(
            static_cast<int>(std::round(*mPlayheadInfoStartRecord->getBpm())));
    else if (mCurrentTempo > 0)
        return std::to_string(static_cast<int>(std::round(mCurrentTempo.load())));
    else
        return "-";
}

std::string NeuralNoteAudioProcessor::getTimeSignatureStr() const
{
    if (mPlayheadInfoStartRecord.hasValue()
        && mPlayheadInfoStartRecord->getTimeSignature().hasValue())
    {
        int num = mPlayheadInfoStartRecord->getTimeSignature()->numerator;
        int denom = mPlayheadInfoStartRecord->getTimeSignature()->denominator;
        return std::to_string(num) + " / " + std::to_string(denom);
    }
    else if (mCurrentTimeSignatureNum > 0 && mCurrentTimeSignatureDenom > 0)
        return std::to_string(mCurrentTimeSignatureNum.load()) + " / "
               + std::to_string(mCurrentTimeSignatureDenom.load());
    else
        return "- / -";
}

void NeuralNoteAudioProcessor::setMidiFileTempo(double inMidiFileTempo)
{
    mMidiFileTempo = inMidiFileTempo;
}

double NeuralNoteAudioProcessor::getMidiFileTempo() const
{
    return mMidiFileTempo;
}

bool NeuralNoteAudioProcessor::isJobRunningOrQueued() const
{
    return mThreadPool.getNumJobs() > 0;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NeuralNoteAudioProcessor();
}
