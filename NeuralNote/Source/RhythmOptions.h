//
// Created by Damien Ronssin on 19.03.23.
//

#ifndef RhythmOptions_h
#define RhythmOptions_h

#include "JuceHeader.h"
#include "Notes.h"
#include "RhythmUtils.h"

class NeuralNoteAudioProcessor;

class RhythmOptions
{
public:
    struct Parameters {
        RhythmUtils::TimeDivisions division = RhythmUtils::_1_4;
        float quantizationForce = 0.f;
    };

    explicit RhythmOptions(NeuralNoteAudioProcessor* inProcessor);

    void processBlock();

    void setInfo(bool inDroppedFile, const Optional<AudioPlayHead::PositionInfo>& inPositionInfoPtr = nullopt);

    bool canQuantize() const;

    void setParameters(RhythmUtils::TimeDivisions inDivision, float inQuantizationForce);

    std::vector<Notes::Event> quantize(const std::vector<Notes::Event>& inNoteEvents);

    void reset();

    void clear();

    const Optional<AudioPlayHead::PositionInfo>& getPlayheadInfoOnRecordStart() const;

    double getCurrentTempo() const;

    std::string getTempoStr() const;

    std::string getTimeSignatureStr() const;

private:
    static double quantizeTime(
        double inEventTime, double inBPM, double inTimeDivision, double inStartTimeQN, float inQuantizationForce);

    struct RhythmInfo {
        Optional<double> bpm;
        Optional<AudioPlayHead::TimeSignature> timeSignature;
        Optional<double> ppqPositionOfLastBarStart;
        Optional<double> ppqPosition;
        bool isPlaying = false;
        bool canQuantize = false;
    };

    NeuralNoteAudioProcessor* mProcessor;

    RhythmInfo mRhythmInfo;

    Parameters mParameters;

    Optional<AudioPlayHead::PositionInfo> mPlayheadInfoStartRecord;

    bool mWasRecording = false;

    std::atomic<double> mCurrentTempo = -1.0;
    std::atomic<int> mCurrentTimeSignatureNum = -1;
    std::atomic<int> mCurrentTimeSignatureDenom = -1;
};

#endif // RhythmOptions_h
