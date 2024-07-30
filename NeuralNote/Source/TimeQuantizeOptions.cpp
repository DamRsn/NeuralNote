//
// Created by Damien Ronssin on 19.03.23.
//

#include "TimeQuantizeOptions.h"
#include "PluginProcessor.h"

TimeQuantizeOptions::TimeQuantizeOptions(NeuralNoteAudioProcessor* inProcessor)
    : mProcessor(inProcessor)
{
    mProcessor->addListenerToStateValueTree(this);
}

TimeQuantizeOptions::~TimeQuantizeOptions()
{
    mProcessor->getValueTree().removeListener(this);
}

void TimeQuantizeOptions::prepareToPlay(double inSampleRate)
{
    mSampleRate = inSampleRate;
}

void TimeQuantizeOptions::processBlock(int inNumSamples)
{
    if (mProcessor->getState() == Recording) {
        if (!mWasRecording) {
            mWasRecording = true;
            auto playhead_info = mProcessor->getPlayHead()->getPosition();
            _setInfo(playhead_info);

            mWasPlaying = isPlayheadPlaying(playhead_info);

            mInfoUpdated = true;
        } else if (!mWasPlaying) {
            auto playhead_info = mProcessor->getPlayHead()->getPosition();

            if (isPlayheadPlaying(playhead_info)) {
                _setInfo(playhead_info);
                mWasPlaying = true;
            }
        }

        mNumRecordedSamples += inNumSamples;
    } else {
        // If we were previously recording but not anymore (user clicked record button to stop it).
        if (mWasRecording) {
            mWasRecording = false;
        }
    }
}

void TimeQuantizeOptions::fileLoaded()
{
    ScopedLock lock(mInfoCriticalSection);
    mTimeQuantizeInfo.refPositionPpq = 0.0f;
    mTimeQuantizeInfo.refBarStartPpq = 0.0f;
    mTimeQuantizeInfo.refPositionSeconds = 0.0f;

    mWasRecording = false;
    mWasPlaying = false;

    saveStateToValueTree();
}

void TimeQuantizeOptions::_setInfo(const Optional<AudioPlayHead::PositionInfo>& inPositionInfoPtr)
{
    ScopedLock lock(mInfoCriticalSection);

    mTimeQuantizeInfo = TimeQuantizeInfo();

    if (inPositionInfoPtr.hasValue()) {
        auto time_signature = inPositionInfoPtr->getTimeSignature();
        if (time_signature.hasValue()) {
            mTimeQuantizeInfo.timeSignatureNum = time_signature->numerator;
            mTimeQuantizeInfo.timeSignatureDenom = time_signature->denominator;
        }

        auto bpm = inPositionInfoPtr->getBpm();

        if (bpm.hasValue()) {
            mTimeQuantizeInfo.bpm = *bpm;
        }

        if (isPlayheadPlaying(inPositionInfoPtr)) {
            if (!inPositionInfoPtr->getPpqPositionOfLastBarStart().hasValue()
                || !inPositionInfoPtr->getPpqPosition().hasValue()) {
                mTimeQuantizeInfo.refBarStartPpq = 0.0;
                mTimeQuantizeInfo.refPositionPpq = 0.0;
                mTimeQuantizeInfo.refPositionSeconds = 0.0;
            } else {
                mTimeQuantizeInfo.refBarStartPpq = *inPositionInfoPtr->getPpqPositionOfLastBarStart();
                mTimeQuantizeInfo.refPositionPpq = *inPositionInfoPtr->getPpqPosition();
                mTimeQuantizeInfo.refPositionSeconds = static_cast<double>(mNumRecordedSamples) / mSampleRate;
            }
        }
    }

    mInfoUpdated = true;
}

void TimeQuantizeOptions::setParameters(bool inEnable,
                                        TimeQuantizeUtils::TimeDivisions inDivision,
                                        float inQuantizationForce)
{
    mParameters.enable = inEnable;
    mParameters.division = inDivision;
    mParameters.quantizationForce = inQuantizationForce;
}

std::vector<Notes::Event> TimeQuantizeOptions::quantize(const std::vector<Notes::Event>& inNoteEvents) const
{
    if (!mParameters.enable) {
        return inNoteEvents;
    }

    std::vector<Notes::Event> out_events;

    const double bpm = mTimeQuantizeInfo.bpm;
    // Offset from previous bar start
    double start_pos_qn = mTimeQuantizeInfo.refPositionPpq - mTimeQuantizeInfo.refPositionSeconds / 60.0 * bpm
                          - mTimeQuantizeInfo.refBarStartPpq;

    double time_division = TimeQuantizeUtils::TimeDivisionsDouble.at(static_cast<size_t>(mParameters.division));

    for (const auto& event: inNoteEvents) {
        double duration = event.endTime - event.startTime;
        jassert(duration > 0);
        double new_start_time =
            _quantizeTime(event.startTime, bpm, time_division, start_pos_qn, mParameters.quantizationForce);
        double new_end_time = new_start_time + duration;

        Notes::Event quantized_event = event;
        quantized_event.startTime = new_start_time;
        quantized_event.endTime = new_end_time;
        out_events.push_back(quantized_event);
    }

    return out_events;
}

void TimeQuantizeOptions::clear()
{
    mTimeQuantizeInfo = TimeQuantizeInfo();
    mWasRecording = false;
}

bool TimeQuantizeOptions::isPlayheadPlaying(const Optional<AudioPlayHead::PositionInfo>& inPositionInfoPtr)
{
    if (inPositionInfoPtr.hasValue()) {
        return inPositionInfoPtr->getIsPlaying();
    }

    return false;
}

void TimeQuantizeOptions::saveStateToValueTree()
{
    ScopedLock lock(mInfoCriticalSection);
    const MessageManagerLock mmLock;

    auto& tree = mProcessor->getValueTree();
    tree.setPropertyExcludingListener(this, NnId::TempoId, mTimeQuantizeInfo.bpm, nullptr);
    tree.setPropertyExcludingListener(
        this, NnId::TimeSignatureNumeratorId, mTimeQuantizeInfo.timeSignatureNum, nullptr);
    tree.setPropertyExcludingListener(
        this, NnId::TimeSignatureDenominatorId, mTimeQuantizeInfo.timeSignatureDenom, nullptr);
    tree.setPropertyExcludingListener(this, NnId::TimeQuantizeRefPosPPQId, mTimeQuantizeInfo.refPositionPpq, nullptr);
    tree.setPropertyExcludingListener(
        this, NnId::TimeQuantizeRefLastBarPPQId, mTimeQuantizeInfo.refBarStartPpq, nullptr);
    tree.setPropertyExcludingListener(
        this, NnId::TimeQuantizeRefPositionSeconds, mTimeQuantizeInfo.refPositionSeconds, nullptr);
}

bool TimeQuantizeOptions::checkInfoUpdated()
{
    auto out = mInfoUpdated.load();
    mInfoUpdated = false;

    return out;
}

TimeQuantizeOptions::TimeQuantizeInfo TimeQuantizeOptions::getTimeQuantizeInfo() const
{
    return mTimeQuantizeInfo;
}

double TimeQuantizeOptions::_quantizeTime(
    double inEventTime, double inBPM, double inTimeDivision, double inStartTimeQN, float inQuantizationForce)
{
    jassert(inEventTime >= 0.0);
    const double seconds_per_qn = 60.0 / inBPM;

    double division_duration = inTimeDivision * 4.0 * seconds_per_qn;

    // Set previous bar start of recording start as new time origin.
    double new_time_origin = inStartTimeQN * seconds_per_qn;
    double shifted_time = inEventTime + new_time_origin;

    double time_since_previous_division = std::fmod(shifted_time, division_duration);

    // Get the time of the first division tick before the note start
    double previous_division_time = shifted_time - time_since_previous_division;

    double target_time = time_since_previous_division < division_duration / 2.0
                             ? previous_division_time
                             : previous_division_time + division_duration;

    jassert(shifted_time >= previous_division_time && shifted_time < previous_division_time + division_duration);

    double quantized_shifted_time = jmap(static_cast<double>(inQuantizationForce), shifted_time, target_time);

    // Re-shift
    double quantized_time = quantized_shifted_time - new_time_origin;

    return quantized_time;
}

void TimeQuantizeOptions::valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property)
{
    if (property == NnId::TempoId) {
        ScopedLock lock(mInfoCriticalSection);
        mTimeQuantizeInfo.bpm = mProcessor->getValueTree().getProperty(property);

    } else if (property == NnId::TimeSignatureNumeratorId) {
        ScopedLock lock(mInfoCriticalSection);
        mTimeQuantizeInfo.timeSignatureNum = mProcessor->getValueTree().getProperty(property);

    } else if (property == NnId::TimeSignatureDenominatorId) {
        ScopedLock lock(mInfoCriticalSection);
        mTimeQuantizeInfo.timeSignatureDenom = mProcessor->getValueTree().getProperty(property);

    } else if (property == NnId::TimeQuantizeRefPosPPQId) {
        ScopedLock lock(mInfoCriticalSection);
        mTimeQuantizeInfo.refPositionPpq = mProcessor->getValueTree().getProperty(property);

    } else if (property == NnId::TimeQuantizeRefLastBarPPQId) {
        ScopedLock lock(mInfoCriticalSection);
        mTimeQuantizeInfo.refBarStartPpq = mProcessor->getValueTree().getProperty(property);

    } else if (property == NnId::TimeQuantizeRefPositionSeconds) {
        ScopedLock lock(mInfoCriticalSection);
        mTimeQuantizeInfo.refPositionSeconds = mProcessor->getValueTree().getProperty(property);
    }
}