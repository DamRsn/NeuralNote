//
// Created by Damien Ronssin on 19.03.23.
//

#ifndef RhythmOptions_h
#define RhythmOptions_h

#include "JuceHeader.h"
#include "Notes.h"
#include "TimeQuantizeUtils.h"

class NeuralNoteAudioProcessor;

class TimeQuantizeOptions : public ValueTree::Listener
{
public:
    struct Parameters {
        TimeQuantizeUtils::TimeDivisions division = TimeQuantizeUtils::_1_4;
        float quantizationForce = 0.f;
    };

    explicit TimeQuantizeOptions(NeuralNoteAudioProcessor* inProcessor);

    ~TimeQuantizeOptions() override;

    void processBlock();

    void setInfo(bool inDroppedFile, const Optional<AudioPlayHead::PositionInfo>& inPositionInfoPtr = nullopt);

    bool canQuantize() const;

    void setParameters(TimeQuantizeUtils::TimeDivisions inDivision, float inQuantizationForce);

    std::vector<Notes::Event> quantize(const std::vector<Notes::Event>& inNoteEvents);

    void reset();

    void clear();

    const Optional<AudioPlayHead::PositionInfo>& getPlayheadInfoOnRecordStart() const;

    double getCurrentTempo() const;

    std::string getTempoStr() const;

    std::string getTimeSignatureStr() const;

    void saveStateToValueTree();

private:
    static double quantizeTime(
        double inEventTime, double inBPM, double inTimeDivision, double inStartTimeQN, float inQuantizationForce);

    void valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property) override;

    struct TimeQuantizeInfo {
        Optional<double> bpm;
        Optional<AudioPlayHead::TimeSignature> timeSignature;
        Optional<double> ppqPositionOfLastBarStart;
        Optional<double> ppqPosition;
        bool isPlaying = false;
        bool canQuantize = false;
    };

    NeuralNoteAudioProcessor* mProcessor;

    TimeQuantizeInfo mTimeQuantizeInfo;

    Parameters mParameters;

    Optional<AudioPlayHead::PositionInfo> mPlayheadInfoStartRecord;

    bool mWasRecording = false;

    std::atomic<double> mCurrentTempo = -1.0;
    std::atomic<int> mCurrentTimeSignatureNum = -1;
    std::atomic<int> mCurrentTimeSignatureDenom = -1;
};

#endif // RhythmOptions_h
