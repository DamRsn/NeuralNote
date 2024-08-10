//
// Created by Damien Ronssin on 02.06.2024.
//

#include "TranscriptionManager.h"
#include "PluginProcessor.h"
#include "NeuralNoteMainView.h"

TranscriptionManager::TranscriptionManager(NeuralNoteAudioProcessor* inProcessor)
    : mProcessor(inProcessor)
    , mTimeQuantizeOptions(inProcessor)
    , mThreadPool(1)
{
    mJobLambda = [this] { _runModel(); };

    auto& apvts = mProcessor->getAPVTS();

    apvts.addParameterListener(ParameterHelpers::getIdStr(ParameterHelpers::NoteSensibilityId), this);
    apvts.addParameterListener(ParameterHelpers::getIdStr(ParameterHelpers::SplitSensibilityId), this);
    apvts.addParameterListener(ParameterHelpers::getIdStr(ParameterHelpers::MinimumNoteDurationId), this);
    apvts.addParameterListener(ParameterHelpers::getIdStr(ParameterHelpers::PitchBendModeId), this);

    apvts.addParameterListener(ParameterHelpers::getIdStr(ParameterHelpers::EnableNoteQuantizationId), this);
    apvts.addParameterListener(ParameterHelpers::getIdStr(ParameterHelpers::KeyRootNoteId), this);
    apvts.addParameterListener(ParameterHelpers::getIdStr(ParameterHelpers::KeyTypeId), this);
    apvts.addParameterListener(ParameterHelpers::getIdStr(ParameterHelpers::KeySnapModeId), this);
    apvts.addParameterListener(ParameterHelpers::getIdStr(ParameterHelpers::MinMidiNoteId), this);
    apvts.addParameterListener(ParameterHelpers::getIdStr(ParameterHelpers::MaxMidiNoteId), this);

    apvts.addParameterListener(ParameterHelpers::getIdStr(ParameterHelpers::EnableTimeQuantizationId), this);
    apvts.addParameterListener(ParameterHelpers::getIdStr(ParameterHelpers::TimeDivisionId), this);
    apvts.addParameterListener(ParameterHelpers::getIdStr(ParameterHelpers::QuantizationForceId), this);

    startTimerHz(30);
}

void TranscriptionManager::timerCallback()
{
    if (mTimeQuantizeOptions.checkInfoUpdated()) {
        mTimeQuantizeOptions.saveStateToValueTree(true);
    }

    if (mShouldRunNewTranscription) {
        launchTranscribeJob();
        _repaintPianoRoll();
    } else if (mShouldUpdateTranscription) {
        _updateTranscription();
        _repaintPianoRoll();
    } else if (mShouldUpdatePostProcessing) {
        _updatePostProcessing();
        _repaintPianoRoll();
    } else if (mShouldRepaintPianoRoll) {
        _repaintPianoRoll();
    }
}
void TranscriptionManager::prepareToPlay(double inSampleRate)
{
    mTimeQuantizeOptions.prepareToPlay(inSampleRate);
}

void TranscriptionManager::processBlock(int inNumSamples)
{
    mTimeQuantizeOptions.processBlock(inNumSamples);
}

void TranscriptionManager::setLaunchNewTranscription()
{
    mShouldRunNewTranscription = true;
    mShouldUpdateTranscription = false;
    mShouldUpdatePostProcessing = false;
}

void TranscriptionManager::parameterChanged(const String& parameterID, float newValue)
{
    if (mProcessor->getState() == PopulatedAudioAndMidiRegions) {
        if (parameterID == ParameterHelpers::getIdStr(ParameterHelpers::NoteSensibilityId)
            || parameterID == ParameterHelpers::getIdStr(ParameterHelpers::SplitSensibilityId)
            || parameterID == ParameterHelpers::getIdStr(ParameterHelpers::MinimumNoteDurationId)) {
            mProcessor->getAPVTS().getRawParameterValue(parameterID)->store(newValue);
            mShouldUpdateTranscription = true;

        } else if (parameterID == ParameterHelpers::getIdStr(ParameterHelpers::EnableNoteQuantizationId)
                   || parameterID == ParameterHelpers::getIdStr(ParameterHelpers::KeyRootNoteId)
                   || parameterID == ParameterHelpers::getIdStr(ParameterHelpers::KeyTypeId)
                   || parameterID == ParameterHelpers::getIdStr(ParameterHelpers::KeySnapModeId)
                   || parameterID == ParameterHelpers::getIdStr(ParameterHelpers::MinMidiNoteId)
                   || parameterID == ParameterHelpers::getIdStr(ParameterHelpers::MaxMidiNoteId)
                   || parameterID == ParameterHelpers::getIdStr(ParameterHelpers::EnableTimeQuantizationId)
                   || parameterID == ParameterHelpers::getIdStr(ParameterHelpers::TimeDivisionId)
                   || parameterID == ParameterHelpers::getIdStr(ParameterHelpers::QuantizationForceId)) {
            mProcessor->getAPVTS().getRawParameterValue(parameterID)->store(newValue);
            mShouldUpdatePostProcessing = true;
        } else if (parameterID == ParameterHelpers::getIdStr(ParameterHelpers::PitchBendModeId)) {
            mProcessor->getAPVTS().getRawParameterValue(parameterID)->store(newValue);
            mShouldRepaintPianoRoll = true;
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

    mNoteOptions.setParameters(
        mProcessor->getParameterValue(ParameterHelpers::EnableNoteQuantizationId) > 0.5f,
        static_cast<NoteUtils::RootNote>(mProcessor->getParameterValue(ParameterHelpers::KeyRootNoteId)),
        static_cast<NoteUtils::ScaleType>(mProcessor->getParameterValue(ParameterHelpers::KeyTypeId)),
        static_cast<NoteUtils::SnapMode>(mProcessor->getParameterValue(ParameterHelpers::KeySnapModeId)),
        static_cast<int>(mProcessor->getParameterValue(ParameterHelpers::MinMidiNoteId)),
        static_cast<int>(mProcessor->getParameterValue(ParameterHelpers::MaxMidiNoteId)));

    auto post_processed_notes = mNoteOptions.process(mBasicPitch.getNoteEvents());

    mTimeQuantizeOptions.setParameters(
        mProcessor->getParameterValue(ParameterHelpers::EnableTimeQuantizationId) > 0.5f,
        static_cast<TimeQuantizeUtils::TimeDivisions>(mProcessor->getParameterValue(ParameterHelpers::TimeDivisionId)),
        mProcessor->getParameterValue(ParameterHelpers::QuantizationForceId));

    mPostProcessedNotes = mTimeQuantizeOptions.quantize(post_processed_notes);

    Notes::dropOverlappingPitchBends(mPostProcessedNotes);
    Notes::mergeOverlappingNotesWithSamePitch(mPostProcessedNotes);

    // For the synth
    auto single_events = SynthController::buildMidiEventsVector(mPostProcessedNotes);
    mProcessor->getPlayer()->getSynthController()->setNewMidiEventsVectorToUse(single_events);

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
        mNoteOptions.setParameters(
            mProcessor->getParameterValue(ParameterHelpers::EnableNoteQuantizationId) > 0.5f,
            static_cast<NoteUtils::RootNote>(mProcessor->getParameterValue(ParameterHelpers::KeyRootNoteId)),
            static_cast<NoteUtils::ScaleType>(mProcessor->getParameterValue(ParameterHelpers::KeyTypeId)),
            static_cast<NoteUtils::SnapMode>(mProcessor->getParameterValue(ParameterHelpers::KeySnapModeId)),
            static_cast<int>(mProcessor->getParameterValue(ParameterHelpers::MinMidiNoteId)),
            static_cast<int>(mProcessor->getParameterValue(ParameterHelpers::MaxMidiNoteId)));

        // TODO: Make this vector a member to avoid reallocating every time
        auto post_processed_notes = mNoteOptions.process(mBasicPitch.getNoteEvents());

        mTimeQuantizeOptions.setParameters(mProcessor->getParameterValue(ParameterHelpers::EnableTimeQuantizationId)
                                               > 0.5f,
                                           static_cast<TimeQuantizeUtils::TimeDivisions>(
                                               mProcessor->getParameterValue(ParameterHelpers::TimeDivisionId)),
                                           mProcessor->getParameterValue(ParameterHelpers::QuantizationForceId));

        // TODO: Pass mPostProcessedNotes as reference
        mPostProcessedNotes = mTimeQuantizeOptions.quantize(post_processed_notes);

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

TimeQuantizeOptions& TranscriptionManager::getTimeQuantizeOptions()
{
    return mTimeQuantizeOptions;
}

void TranscriptionManager::clear()
{
    mBasicPitch.reset();
    mShouldRunNewTranscription = false;
    mShouldUpdateTranscription = false;
    mShouldUpdatePostProcessing = false;
    mPostProcessedNotes.clear();
    mTimeQuantizeOptions.clear();
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

void TranscriptionManager::_repaintPianoRoll()
{
    auto* main_view = mProcessor->getNeuralNoteMainView();

    if (main_view) {
        main_view->repaintPianoRoll();
    }

    mShouldRepaintPianoRoll = false;
}
