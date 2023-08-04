#include "PluginProcessor.h"
#include "PluginEditor.h"

NeuralNoteAudioProcessor::NeuralNoteAudioProcessor()
    : mTree(*this, nullptr, "PARAMETERS", createParameterLayout())
    , mThreadPool(1)
{
    mJobLambda = [this]() { _runModel(); };

    mSourceAudioManager = std::make_unique<SourceAudioManager>(this);
    mPlayer = std::make_unique<Player>(this);
}

AudioProcessorValueTreeState::ParameterLayout NeuralNoteAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto mute = std::make_unique<juce::AudioParameterBool>(juce::ParameterID {"MUTE", 1}, "Mute", true);
    params.push_back(std::move(mute));

    auto audio_level_db = std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"AUDIO_LEVEL_DB", 1}, "Audio Level dB", NormalisableRange<float>(-36.f, 6.0f, 1.0f), 0.0f);
    params.push_back(std::move(audio_level_db));

    auto midi_level_db = std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"MIDI_LEVEL_DB", 1}, "Midi Level dB", NormalisableRange<float>(-36.f, 6.0f, 1.0f), 0.0f);
    params.push_back(std::move(midi_level_db));

    return {params.begin(), params.end()};
}

void NeuralNoteAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    mSourceAudioManager->prepareToPlay(sampleRate, samplesPerBlock);
    mPlayer->prepareToPlay(sampleRate, samplesPerBlock);
}

void NeuralNoteAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);

    // Get tempo and time signature for UI.
    auto playhead_info = getPlayHead()->getPosition();
    if (playhead_info.hasValue()) {
        if (playhead_info->getBpm().hasValue())
            mCurrentTempo = *playhead_info->getBpm();
        if (playhead_info->getTimeSignature().hasValue()) {
            mCurrentTimeSignatureNum = playhead_info->getTimeSignature()->numerator;
            mCurrentTimeSignatureDenom = playhead_info->getTimeSignature()->denominator;
        }
    }

    mSourceAudioManager->processBlock(buffer);

    if (mState.load() == Recording) {
        if (!mWasRecording) {
            mWasRecording = true;
            mPlayheadInfoStartRecord = getPlayHead()->getPosition();

            mRhythmOptions.setInfo(false, mPlayheadInfoStartRecord);
        }
    } else {
        // If we were previously recording but not anymore (user clicked record button to stop it).
        if (mWasRecording) {
            mWasRecording = false;
        }
    }

    auto isMute = mTree.getRawParameterValue("MUTE")->load() > 0.5;

    if (isMute)
        buffer.clear();

    mPlayer->processBlock(buffer);
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
    mPostProcessedNotes.clear();
    mPlayheadInfoStartRecord = juce::Optional<juce::AudioPlayHead::PositionInfo>();

    mCurrentTempo = -1;
    mCurrentTimeSignatureNum = -1;
    mCurrentTimeSignatureDenom = -1;

    mMidiFileTempo = 120.0;

    mBasicPitch.reset();
    mWasRecording = false;

    mPlayer->reset();
    mSourceAudioManager->clear();

    mRhythmOptions.reset();

    mState.store(EmptyAudioAndMidiRegions);
}

void NeuralNoteAudioProcessor::launchTranscribeJob()
{
    jassert(mState.load() == Processing);

    // Have at least one second to transcribe
    if (getSourceAudioManager()->getNumSamplesDownAcquired() >= 1 * AUDIO_SAMPLE_RATE) {
        mThreadPool.addJob(mJobLambda);
    } else {
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

const juce::Optional<juce::AudioPlayHead::PositionInfo>& NeuralNoteAudioProcessor::getPlayheadInfoOnRecordStart()
{
    return mPlayheadInfoStartRecord;
}

void NeuralNoteAudioProcessor::_runModel()
{
    mBasicPitch.setParameters(mParameters.noteSensibility, mParameters.splitSensibility, mParameters.minNoteDurationMs);

    mBasicPitch.transcribeToMIDI(
        getSourceAudioManager()->getDownsampledSourceAudioForTranscription().getWritePointer(0),
        getSourceAudioManager()->getNumSamplesDownAcquired());

    mNoteOptions.setParameters(NoteUtils::RootNote(mParameters.keyRootNote.load()),
                               NoteUtils::ScaleType(mParameters.keyType.load()),
                               NoteUtils::SnapMode(mParameters.keySnapMode.load()),
                               mParameters.minMidiNote.load(),
                               mParameters.maxMidiNote.load());

    auto post_processed_notes = mNoteOptions.process(mBasicPitch.getNoteEvents());

    mRhythmOptions.setParameters(RhythmUtils::TimeDivisions(mParameters.rhythmTimeDivision.load()),
                                 mParameters.rhythmQuantizationForce.load());

    mPostProcessedNotes = mRhythmOptions.quantize(post_processed_notes);

    Notes::dropOverlappingPitchBends(mPostProcessedNotes);
    Notes::mergeOverlappingNotesWithSamePitch(mPostProcessedNotes);

    // For the synth
    auto single_events = SynthController::buildMidiEventsVector(mPostProcessedNotes);
    mPlayer->getSynthController()->setNewMidiEventsVectorToUse(single_events);

    mMidiFileTempo = mCurrentTempo.load() > 0 ? mCurrentTempo.load() : 120;

    mState.store(PopulatedAudioAndMidiRegions);
}

void NeuralNoteAudioProcessor::updateTranscription()
{
    jassert(mState == PopulatedAudioAndMidiRegions);

    if (mState == PopulatedAudioAndMidiRegions) {
        mBasicPitch.setParameters(
            mParameters.noteSensibility, mParameters.splitSensibility, mParameters.minNoteDurationMs);

        mBasicPitch.updateMIDI();
        updatePostProcessing();
    }
}

void NeuralNoteAudioProcessor::updatePostProcessing()
{
    jassert(mState == PopulatedAudioAndMidiRegions);

    if (mState == PopulatedAudioAndMidiRegions) {
        mNoteOptions.setParameters(NoteUtils::RootNote(mParameters.keyRootNote.load()),
                                   NoteUtils::ScaleType(mParameters.keyType.load()),
                                   NoteUtils::SnapMode(mParameters.keySnapMode.load()),
                                   mParameters.minMidiNote.load(),
                                   mParameters.maxMidiNote.load());

        auto post_processed_notes = mNoteOptions.process(mBasicPitch.getNoteEvents());

        mRhythmOptions.setParameters(RhythmUtils::TimeDivisions(mParameters.rhythmTimeDivision.load()),
                                     mParameters.rhythmQuantizationForce.load());

        mPostProcessedNotes = mRhythmOptions.quantize(post_processed_notes);

        Notes::dropOverlappingPitchBends(mPostProcessedNotes);
        Notes::mergeOverlappingNotesWithSamePitch(mPostProcessedNotes);

        // For the synth
        auto single_events = SynthController::buildMidiEventsVector(mPostProcessedNotes);
        mPlayer->getSynthController()->setNewMidiEventsVectorToUse(single_events);
    }
}

bool NeuralNoteAudioProcessor::canQuantize() const
{
    return mRhythmOptions.canPerformQuantization();
}

std::string NeuralNoteAudioProcessor::getTempoStr() const
{
    if (mPlayheadInfoStartRecord.hasValue() && mPlayheadInfoStartRecord->getBpm().hasValue())
        return std::to_string(static_cast<int>(std::round(*mPlayheadInfoStartRecord->getBpm())));
    else if (mCurrentTempo > 0)
        return std::to_string(static_cast<int>(std::round(mCurrentTempo.load())));
    else
        return "-";
}

std::string NeuralNoteAudioProcessor::getTimeSignatureStr() const
{
    if (mPlayheadInfoStartRecord.hasValue() && mPlayheadInfoStartRecord->getTimeSignature().hasValue()) {
        int num = mPlayheadInfoStartRecord->getTimeSignature()->numerator;
        int denom = mPlayheadInfoStartRecord->getTimeSignature()->denominator;
        return std::to_string(num) + " / " + std::to_string(denom);
    } else if (mCurrentTimeSignatureNum > 0 && mCurrentTimeSignatureDenom > 0)
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

Player* NeuralNoteAudioProcessor::getPlayer()
{
    return mPlayer.get();
}

SourceAudioManager* NeuralNoteAudioProcessor::getSourceAudioManager()
{
    return mSourceAudioManager.get();
}

RhythmOptions* NeuralNoteAudioProcessor::getRhythmOptions()
{
    return &mRhythmOptions;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NeuralNoteAudioProcessor();
}
