//
// Created by Damien Ronssin on 19.03.23.
//

#ifndef RhythmOptions_h
#define RhythmOptions_h

#include <JuceHeader.h>

#include "Notes.h"
#include "RhythmUtils.h"

class RhythmOptions
{
public:
    struct Parameters
    {
        RhythmUtils::TimeDivisions division = RhythmUtils::_1_4;
        float quantizationForce = 0.f;
    };

    RhythmOptions();

    void setInfo(bool inDroppedFile,
                 const juce::Optional<juce::AudioPlayHead::PositionInfo>&
                     inPositionInfoPtr = nullopt);

    bool canPerformQuantization() const;

    void setParameters(RhythmUtils::TimeDivisions inDivision, float inQuantizationForce);

    std::vector<Notes::Event> quantize(const std::vector<Notes::Event>& inNoteEvents);

private:
    static double quantizeTime(double inEventTime,
                               double inBPM,
                               double inTimeDivision,
                               double inStartTimeQN,
                               float inQuantizationForce);

    struct RhythmInfo
    {
        bool droppedFile = false;
        juce::Optional<double> bpm;
        juce::Optional<juce::AudioPlayHead::TimeSignature> timeSignature;
        juce::Optional<double> ppqPositionOfLastBarStart;
        juce::Optional<double> ppqPosition;
        bool isPlaying = false;
        bool canQuantize = false;
    };

    RhythmInfo mRhythmInfo;

    Parameters mParameters;
};

#endif // RhythmOptions_h
