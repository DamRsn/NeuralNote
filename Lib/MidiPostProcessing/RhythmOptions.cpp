//
// Created by Damien Ronssin on 19.03.23.
//

#include "RhythmOptions.h"
RhythmOptions::RhythmOptions()
{
}

void RhythmOptions::setInfo(
    bool inDroppedFile,
    const juce::Optional<juce::AudioPlayHead::PositionInfo>& inPositionInfoPtr)
{
    // Reset to default struct where nothing is set and bool false.
    mRhythmInfo = RhythmInfo();

    if (inDroppedFile)
    {
        mRhythmInfo.droppedFile = true;
        mRhythmInfo.canQuantize = false;
    }
    else
    {
        mRhythmInfo.droppedFile = false;

        if (inPositionInfoPtr.hasValue())
        {
            mRhythmInfo.timeSignature = inPositionInfoPtr->getTimeSignature();
            mRhythmInfo.isPlaying = inPositionInfoPtr->getIsPlaying();
            mRhythmInfo.bpm = inPositionInfoPtr->getBpm();
            mRhythmInfo.ppqPositionOfLastBarStart =
                inPositionInfoPtr->getPpqPositionOfLastBarStart();
            mRhythmInfo.ppqPosition = inPositionInfoPtr->getPpqPosition();
            // Can quantize only if recorded while playing, bpm is defined, lastBarStart is defined ...
            mRhythmInfo.canQuantize = mRhythmInfo.isPlaying && mRhythmInfo.bpm.hasValue()
                                      && mRhythmInfo.ppqPositionOfLastBarStart.hasValue()
                                      && mRhythmInfo.ppqPosition
                                      && mRhythmInfo.timeSignature.hasValue();
        }
        else
        {
            mRhythmInfo.canQuantize = false;
        }
    }
}

bool RhythmOptions::canPerformQuantization() const
{
    return mRhythmInfo.canQuantize;
}

void RhythmOptions::setParameters(RhythmUtils::TimeDivisions inDivision,
                                  float inQuantizationForce)
{
    mParameters.division = inDivision;
    mParameters.quantizationForce = inQuantizationForce;
}

std::vector<Notes::Event>
    RhythmOptions::quantize(const std::vector<Notes::Event>& inNoteEvents)
{
    std::vector<Notes::Event> out_events;

    if (!mRhythmInfo.canQuantize)
    {
        out_events = inNoteEvents;
        return out_events;
    }

    double bpm = *mRhythmInfo.bpm;
    // Offset from previous bar start
    double start_pos_qn =
        *mRhythmInfo.ppqPosition - *mRhythmInfo.ppqPositionOfLastBarStart;

    double time_division =
        RhythmUtils::TimeDivisionsDouble.at(static_cast<size_t>(mParameters.division));

    for (const auto& event: inNoteEvents)
    {
        double duration = event.endTime - event.startTime;
        jassert(duration > 0);
        double new_start_time = quantizeTime(event.startTime,
                                             bpm,
                                             time_division,
                                             start_pos_qn,
                                             mParameters.quantizationForce);
        double new_end_time = new_start_time + duration;

        Notes::Event quantized_event = event;
        quantized_event.startTime = new_start_time;
        quantized_event.endTime = new_end_time;
        out_events.push_back(quantized_event);
    }

    return out_events;
}

double RhythmOptions::quantizeTime(double inEventTime,
                                   double inBPM,
                                   double inTimeDivision,
                                   double inStartTimeQN,
                                   float inQuantizationForce)
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

    jassert(shifted_time >= previous_division_time
            && shifted_time < previous_division_time + division_duration);

    double quantized_shifted_time =
        jmap(double(inQuantizationForce), shifted_time, target_time);

    // Re-shift
    double quantized_time = quantized_shifted_time - new_time_origin;

    return quantized_time;
}
