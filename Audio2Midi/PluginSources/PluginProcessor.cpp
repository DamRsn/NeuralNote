#include "PluginProcessor.h"
#include "PluginEditor.h"

Audio2MidiAudioProcessor::Audio2MidiAudioProcessor()
    : mTree(*this, nullptr, "PARAMETERS", createParameterLayout())
    , mThreadPool(1)
{
    mAudioBufferForMIDITranscription.setSize(1, mMaxNumSamplesToConvert);
    mAudioBufferForMIDITranscription.clear();

    mJobLambda = [this]() { _runModel(); };
}

AudioProcessorValueTreeState::ParameterLayout
    Audio2MidiAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto mute = std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID {"MUTE", 1}, "Mute", true);
    params.push_back(std::move(mute));

    return {params.begin(), params.end()};
}

void Audio2MidiAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    mDownSampler.prepareToPlay(sampleRate, samplesPerBlock);

    mMonoBuffer.setSize(1, samplesPerBlock);
}

void Audio2MidiAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                            juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    const int num_in_channels = getTotalNumInputChannels();

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

juce::AudioProcessorEditor* Audio2MidiAudioProcessor::createEditor()
{
    return new Audio2MidiEditor(*this);
}

void Audio2MidiAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
}

void Audio2MidiAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
}

void Audio2MidiAudioProcessor::clear()
{
    mNumSamplesAcquired = 0;
    mAudioBufferForMIDITranscription.clear();

    mPostProcessedNotes.clear();
    mPlayheadInfoStartRecord = juce::Optional<juce::AudioPlayHead::PositionInfo>();
    mDroppedFilename = "";

    mBasicPitch.reset();
    mWasRecording = false;
    mIsPlayheadPlaying = false;
    mState.store(EmptyAudioAndMidiRegions);
}

AudioBuffer<float>& Audio2MidiAudioProcessor::getAudioBufferForMidi()
{
    return mAudioBufferForMIDITranscription;
}

int Audio2MidiAudioProcessor::getNumSamplesAcquired() const
{
    return mNumSamplesAcquired;
}

void Audio2MidiAudioProcessor::setNumSamplesAcquired(int inNumSamplesAcquired)
{
    mNumSamplesAcquired = inNumSamplesAcquired;
}

void Audio2MidiAudioProcessor::launchTranscribeJob()
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

const std::vector<Notes::Event>& Audio2MidiAudioProcessor::getNoteEventVector() const
{
    return mPostProcessedNotes;
}

Audio2MidiAudioProcessor::Parameters* Audio2MidiAudioProcessor::getCustomParameters()
{
    return &mParameters;
}

const juce::Optional<juce::AudioPlayHead::PositionInfo>&
    Audio2MidiAudioProcessor::getPlayheadInfoOnRecordStart()
{
    return mPlayheadInfoStartRecord;
}

void Audio2MidiAudioProcessor::_runModel()
{
    mBasicPitch.setParameters(mParameters.noteSensibility,
                              mParameters.splitSensibility,
                              mParameters.minNoteDurationMs,
                              mParameters.pitchBendMode);

    mBasicPitch.transcribeToMIDI(mAudioBufferForMIDITranscription.getWritePointer(0),
                                 mNumSamplesAcquired);

    mNoteOptions.setParameters(NoteUtils::RootNote(mParameters.keyRootNote.load()),
                               NoteUtils::ScaleType(mParameters.keyType.load()),
                               NoteUtils::SnapMode(mParameters.keySnapMode.load()),
                               mParameters.minMidiNote.load(),
                               mParameters.maxMidiNote.load());

    auto post_processed_notes = mNoteOptions.processKey(mBasicPitch.getNoteEvents());

    mRhythmOptions.setParameters(
        RhythmUtils::TimeDivisions(mParameters.rhythmTimeDivision.load()),
        mParameters.rhythmQuantizationForce.load(),
        false);

    mPostProcessedNotes = mRhythmOptions.quantize(post_processed_notes);

    mState.store(PopulatedAudioAndMidiRegions);
}

void Audio2MidiAudioProcessor::updateTranscription()
{
    jassert(mState == PopulatedAudioAndMidiRegions);

    if (mState == PopulatedAudioAndMidiRegions)
    {
        mBasicPitch.setParameters(mParameters.noteSensibility,
                                  mParameters.splitSensibility,
                                  mParameters.minNoteDurationMs,
                                  mParameters.pitchBendMode);

        mBasicPitch.updateMIDI();
        updatePostProcessing();
    }
}

void Audio2MidiAudioProcessor::updatePostProcessing()
{
    jassert(mState == PopulatedAudioAndMidiRegions);

    if (mState == PopulatedAudioAndMidiRegions)
    {
        mNoteOptions.setParameters(NoteUtils::RootNote(mParameters.keyRootNote.load()),
                                   NoteUtils::ScaleType(mParameters.keyType.load()),
                                   NoteUtils::SnapMode(mParameters.keySnapMode.load()),
                                   mParameters.minMidiNote.load(),
                                   mParameters.maxMidiNote.load());

        auto post_processed_notes = mNoteOptions.processKey(mBasicPitch.getNoteEvents());

        mRhythmOptions.setParameters(
            RhythmUtils::TimeDivisions(mParameters.rhythmTimeDivision.load()),
            mParameters.rhythmQuantizationForce.load(),
            false);

        mPostProcessedNotes = mRhythmOptions.quantize(post_processed_notes);
    }
}
void Audio2MidiAudioProcessor::setFileDrop(const std::string& inFilename)
{
    mRhythmOptions.setInfo(true);
    mDroppedFilename = inFilename;
}

std::string Audio2MidiAudioProcessor::getDroppedFilename()
{
    return mDroppedFilename;
}

bool Audio2MidiAudioProcessor::canQuantize()
{
    return mRhythmOptions.canPerformQuantization();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Audio2MidiAudioProcessor();
}
