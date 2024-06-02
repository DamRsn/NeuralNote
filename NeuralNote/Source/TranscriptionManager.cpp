//
// Created by Damien Ronssin on 02.06.2024.
//

#include "TranscriptionManager.h"
#include "PluginProcessor.h"

TranscriptionManager::TranscriptionManager(NeuralNoteAudioProcessor* inProcessor)
    : mProcessor(inProcessor)
    , mThreadPool(1)
{
    mJobLambda = [this]() { _runModel(); };

    startTimerHz(30);
}

void TranscriptionManager::timerCallback()
{
    // TODO: should repaint pianoRoll here somehow
    if (mShouldRunNewTranscription) {
        launchTranscribeJob();
    } else if (mShouldUpdateTranscription) {
        _updateTranscription();
    } else if (mShouldUpdatePostProcessing) {
        _updatePostProcessing();
    }
}

void TranscriptionManager::setLauchNewTranscription()
{
    mShouldRunNewTranscription = true;
    mShouldUpdateTranscription = false;
    mShouldUpdatePostProcessing = false;
}

void TranscriptionManager::parameterChanged(const String& parameterID, float newValue)
{
    if (mProcessor->getState() == PopulatedAudioAndMidiRegions) {
        if (parameterID == ParameterHelpers::toIdStr(ParameterHelpers::NoteSensibilityId)
            || parameterID == ParameterHelpers::toIdStr(ParameterHelpers::SplitSensibilityId)
            || parameterID == ParameterHelpers::toIdStr(ParameterHelpers::MinimumNoteDurationId)) {
            mShouldUpdateTranscription = true;
        } else if (parameterID == ParameterHelpers::toIdStr(ParameterHelpers::KeyRootNoteId)
                   || parameterID == ParameterHelpers::toIdStr(ParameterHelpers::KeyTypeId)
                   || parameterID == ParameterHelpers::toIdStr(ParameterHelpers::KeySnapModeId)
                   || parameterID == ParameterHelpers::toIdStr(ParameterHelpers::MinMidiNoteId)
                   || parameterID == ParameterHelpers::toIdStr(ParameterHelpers::MaxMidiNoteId)
                   || parameterID == ParameterHelpers::toIdStr(ParameterHelpers::TimeDivisionId)
                   || parameterID == ParameterHelpers::toIdStr(ParameterHelpers::QuantizationForceId)) {
            mShouldUpdatePostProcessing = true;
        }
    }
}

void TranscriptionManager::_runModel()
{
    mBasicPitch.setParameters(mProcessor->getParameterValue(ParameterHelpers::NoteSensibilityId),
                              mProcessor->getParameterValue(ParameterHelpers::SplitSensibilityId),
                              mProcessor->getParameterValue(ParameterHelpers::MinimumNoteDurationId));
    mBasicPitch.transcribeToMIDI(
        mProcessor->getSourceAudioManager()->getDownsampledSourceAudioForTranscription().getWritePointer(0),
        mProcessor->getSourceAudioManager()->getNumSamplesDownAcquired());

    mNoteOptions.setParameters(NoteUtils::RootNote(mProcessor->getParameterValue(ParameterHelpers::KeyRootNoteId)),
                               NoteUtils::ScaleType(mProcessor->getParameterValue(ParameterHelpers::KeyTypeId)),
                               NoteUtils::SnapMode(mProcessor->getParameterValue(ParameterHelpers::KeySnapModeId)),
                               (int) mProcessor->getParameterValue(ParameterHelpers::MinMidiNoteId),
                               (int) mProcessor->getParameterValue(ParameterHelpers::MaxMidiNoteId));

    auto post_processed_notes = mNoteOptions.process(mBasicPitch.getNoteEvents());

    mRhythmOptions.setParameters(
        RhythmUtils::TimeDivisions(mProcessor->getParameterValue(ParameterHelpers::TimeDivisionId)),
        mProcessor->getParameterValue(ParameterHelpers::QuantizationForceId));

    mPostProcessedNotes = mRhythmOptions.quantize(post_processed_notes);

    Notes::dropOverlappingPitchBends(mPostProcessedNotes);
    Notes::mergeOverlappingNotesWithSamePitch(mPostProcessedNotes);

    // For the synth
    auto single_events = SynthController::buildMidiEventsVector(mPostProcessedNotes);
    mProcessor->getPlayer()->getSynthController()->setNewMidiEventsVectorToUse(single_events);

    //    mMidiFileTempo = mCurrentTempo.load() > 0 ? mCurrentTempo.load() : 120;

    mProcessor->setStateToPopulatedAudioAndMidiRegions();
}

void TranscriptionManager::_updateTranscription()
{
    jassert(mProcessor->getState() == PopulatedAudioAndMidiRegions);

    if (mProcessor->getState() == PopulatedAudioAndMidiRegions) {
        mBasicPitch.setParameters(mProcessor->getParameterValue(ParameterHelpers::NoteSensibilityId),
                                  mProcessor->getParameterValue(ParameterHelpers::SplitSensibilityId),
                                  mProcessor->getParameterValue(ParameterHelpers::MinimumNoteDurationId));

        mBasicPitch.updateMIDI();
        _updatePostProcessing();
    }

    mShouldUpdateTranscription = false;
    mShouldUpdatePostProcessing = false;
}

void TranscriptionManager::_updatePostProcessing()
{
    jassert(mProcessor->getState() == PopulatedAudioAndMidiRegions);

    if (mProcessor->getState() == PopulatedAudioAndMidiRegions) {
        mNoteOptions.setParameters(NoteUtils::RootNote(mProcessor->getParameterValue(ParameterHelpers::KeyRootNoteId)),
                                   NoteUtils::ScaleType(mProcessor->getParameterValue(ParameterHelpers::KeyTypeId)),
                                   NoteUtils::SnapMode(mProcessor->getParameterValue(ParameterHelpers::KeySnapModeId)),
                                   (int) mProcessor->getParameterValue(ParameterHelpers::MinMidiNoteId),
                                   (int) mProcessor->getParameterValue(ParameterHelpers::MaxMidiNoteId));

        // TODO: Make this vector a member to avoid reallocating every time
        auto post_processed_notes = mNoteOptions.process(mBasicPitch.getNoteEvents());

        mRhythmOptions.setParameters(
            RhythmUtils::TimeDivisions(mProcessor->getParameterValue(ParameterHelpers::TimeDivisionId)),
            mProcessor->getParameterValue(ParameterHelpers::QuantizationForceId));

        // TODO: Pass mPostProcessedNotes as reference
        mPostProcessedNotes = mRhythmOptions.quantize(post_processed_notes);

        Notes::dropOverlappingPitchBends(mPostProcessedNotes);
        Notes::mergeOverlappingNotesWithSamePitch(mPostProcessedNotes);

        // For the synth
        auto single_events = SynthController::buildMidiEventsVector(mPostProcessedNotes);
        mProcessor->getPlayer()->getSynthController()->setNewMidiEventsVectorToUse(single_events);
    }

    mShouldUpdatePostProcessing = false;
}

bool TranscriptionManager::isJobRunningOrQueued() const
{
    return mThreadPool.getNumJobs() > 0;
}

const std::vector<Notes::Event>& TranscriptionManager::getNoteEventVector() const
{
    return mPostProcessedNotes;
}

RhythmOptions& TranscriptionManager::getRhythmOptions()
{
    return mRhythmOptions;
}

void TranscriptionManager::clear()
{
    mBasicPitch.reset();
    mShouldRunNewTranscription = false;
    mShouldUpdateTranscription = false;
    mShouldUpdatePostProcessing = false;
    mPostProcessedNotes.clear();
    mMidiFileTempo = 120.0;
}

void TranscriptionManager::launchTranscribeJob()
{
    jassert(MessageManager::getInstance()->isThisTheMessageThread());
    mProcessor->setStateToProcessing();

    // Have at least one second to transcribe
    if (mProcessor->getSourceAudioManager()->getNumSamplesDownAcquired() >= 1 * AUDIO_SAMPLE_RATE) {
        mThreadPool.addJob(mJobLambda);
    } else {
        mProcessor->clear();
    }

    mShouldRunNewTranscription = false;
    mShouldUpdateTranscription = false;
    mShouldUpdatePostProcessing = false;
}

void TranscriptionManager::setMidiFileTempo(double inMidiFileTempo)
{
    mMidiFileTempo = inMidiFileTempo;
}

double TranscriptionManager::getMidiFileTempo() const
{
    return mMidiFileTempo;
}
