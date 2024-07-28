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

void TimeQuantizeOptions::processBlock()
{
    if (mProcessor->getState() == Recording) {
        if (!mWasRecording) {
            mWasRecording = true;
            // TODO: Set playhead info later if not playing at the start of recording.
            setInfo(false, mProcessor->getPlayHead()->getPosition());
        }
    } else {
        // If we were previously recording but not anymore (user clicked record button to stop it).
        if (mWasRecording) {
            mWasRecording = false;
        }
    }

    // Get tempo and time signature for UI.
    auto playhead_info = mProcessor->getPlayHead()->getPosition();
    if (playhead_info.hasValue()) {
        if (playhead_info->getBpm().hasValue())
            mCurrentTempo = *playhead_info->getBpm();
        if (playhead_info->getTimeSignature().hasValue()) {
            mCurrentTimeSignatureNum = playhead_info->getTimeSignature()->numerator;
            mCurrentTimeSignatureDenom = playhead_info->getTimeSignature()->denominator;
        }
    }
}
void TimeQuantizeOptions::setInfo(bool inDroppedFile, const Optional<AudioPlayHead::PositionInfo>& inPositionInfoPtr)
{
    reset();

    if (inDroppedFile) {
        mTimeQuantizeInfo.canQuantize = false;
    } else {
        if (inPositionInfoPtr.hasValue()) {
            mPlayheadInfoStartRecord = inPositionInfoPtr;

            mTimeQuantizeInfo.timeSignature = inPositionInfoPtr->getTimeSignature();
            mTimeQuantizeInfo.isPlaying = inPositionInfoPtr->getIsPlaying();
            mTimeQuantizeInfo.bpm = inPositionInfoPtr->getBpm();
            mTimeQuantizeInfo.ppqPositionOfLastBarStart = inPositionInfoPtr->getPpqPositionOfLastBarStart();
            mTimeQuantizeInfo.ppqPosition = inPositionInfoPtr->getPpqPosition();
            // Can quantize only if recorded while playing, bpm is defined, lastBarStart is defined ...
            // TODO: allow quantization if not playing at the start of recording (but has to be playing at some point)
            mTimeQuantizeInfo.canQuantize = mTimeQuantizeInfo.isPlaying && mTimeQuantizeInfo.bpm.hasValue()
                                            && mTimeQuantizeInfo.ppqPositionOfLastBarStart.hasValue()
                                            && mTimeQuantizeInfo.ppqPosition
                                            && mTimeQuantizeInfo.timeSignature.hasValue();
        } else {
            mTimeQuantizeInfo.canQuantize = false;
        }
    }
}

bool TimeQuantizeOptions::canQuantize() const
{
    return mTimeQuantizeInfo.canQuantize;
}

void TimeQuantizeOptions::setParameters(TimeQuantizeUtils::TimeDivisions inDivision, float inQuantizationForce)
{
    mParameters.division = inDivision;
    mParameters.quantizationForce = inQuantizationForce;
}

std::vector<Notes::Event> TimeQuantizeOptions::quantize(const std::vector<Notes::Event>& inNoteEvents)
{
    std::vector<Notes::Event> out_events;

    if (!mTimeQuantizeInfo.canQuantize) {
        out_events = inNoteEvents;
        return out_events;
    }

    double bpm = *mTimeQuantizeInfo.bpm;
    // Offset from previous bar start
    double start_pos_qn = *mTimeQuantizeInfo.ppqPosition - *mTimeQuantizeInfo.ppqPositionOfLastBarStart;

    double time_division = TimeQuantizeUtils::TimeDivisionsDouble.at(static_cast<size_t>(mParameters.division));

    for (const auto& event: inNoteEvents) {
        double duration = event.endTime - event.startTime;
        jassert(duration > 0);
        double new_start_time =
            quantizeTime(event.startTime, bpm, time_division, start_pos_qn, mParameters.quantizationForce);
        double new_end_time = new_start_time + duration;

        Notes::Event quantized_event = event;
        quantized_event.startTime = new_start_time;
        quantized_event.endTime = new_end_time;
        out_events.push_back(quantized_event);
    }

    return out_events;
}

void TimeQuantizeOptions::reset()
{
    // Reset to default struct where nothing is set and bool false.
    mTimeQuantizeInfo = TimeQuantizeInfo();
    mPlayheadInfoStartRecord = Optional<AudioPlayHead::PositionInfo>();
}

void TimeQuantizeOptions::clear()
{
    mTimeQuantizeInfo = TimeQuantizeInfo();
    mPlayheadInfoStartRecord = Optional<AudioPlayHead::PositionInfo>();

    mCurrentTempo = -1;
    mCurrentTimeSignatureNum = -1;
    mCurrentTimeSignatureDenom = -1;
    mWasRecording = false;
}

const Optional<AudioPlayHead::PositionInfo>& TimeQuantizeOptions::getPlayheadInfoOnRecordStart() const
{
    return mPlayheadInfoStartRecord;
}

double TimeQuantizeOptions::getCurrentTempo() const
{
    return mCurrentTempo.load();
}

std::string TimeQuantizeOptions::getTempoStr() const
{
    if (mPlayheadInfoStartRecord.hasValue() && mPlayheadInfoStartRecord->getBpm().hasValue()) {
        return std::to_string(static_cast<int>(std::round(*mPlayheadInfoStartRecord->getBpm())));
    }

    if (mCurrentTempo > 0) {
        return std::to_string(static_cast<int>(std::round(mCurrentTempo.load())));
    }

    return "-";
}

std::string TimeQuantizeOptions::getTimeSignatureStr() const
{
    if (mPlayheadInfoStartRecord.hasValue() && mPlayheadInfoStartRecord->getTimeSignature().hasValue()) {
        int num = mPlayheadInfoStartRecord->getTimeSignature()->numerator;
        int denom = mPlayheadInfoStartRecord->getTimeSignature()->denominator;
        return std::to_string(num) + " / " + std::to_string(denom);
    }

    if (mCurrentTimeSignatureNum > 0 && mCurrentTimeSignatureDenom > 0) {
        return std::to_string(mCurrentTimeSignatureNum.load()) + " / "
               + std::to_string(mCurrentTimeSignatureDenom.load());
    }

    return "- / -";
}

void TimeQuantizeOptions::saveStateToValueTree()
{
    mProcessor->getValueTree().setPropertyExcludingListener(this, NnId::TempoId, mCurrentTempo.load(), nullptr);
    mProcessor->getValueTree().setPropertyExcludingListener(
        this, NnId::TimeSignatureNumeratorId, mCurrentTimeSignatureNum.load(), nullptr);
    mProcessor->getValueTree().setPropertyExcludingListener(
        this, NnId::TimeSignatureDenominatorId, mCurrentTimeSignatureDenom.load(), nullptr);
}

double TimeQuantizeOptions::quantizeTime(
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
        mCurrentTempo = mProcessor->getValueTree().getProperty(property);
    } else if (property == NnId::TimeSignatureNumeratorId) {
        mCurrentTimeSignatureNum = mProcessor->getValueTree().getProperty(property);
    } else if (property == NnId::TimeSignatureDenominatorId) {
        mCurrentTimeSignatureDenom = mProcessor->getValueTree().getProperty(property);
    }
}