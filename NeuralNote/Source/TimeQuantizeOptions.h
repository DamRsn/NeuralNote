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
        bool enable = false;
        TimeQuantizeUtils::TimeDivisions division = TimeQuantizeUtils::_1_4;
        float quantizationForce = 0.f;
    };

    struct TimeQuantizeInfo {
        double bpm = 120.0;
        int timeSignatureNum = 4;
        int timeSignatureDenom = 4;

        // Reference bar + position with corresponding time in seconds
        double refBarStartPpq = 0.0;
        double refPositionPpq = 0.0;
        double refPositionSeconds = 0.0;

        /**
         * @return The start position in quarter notes
         */
        double getStartPPQ() const { return refPositionPpq - refPositionSeconds * bpm / 60.0; }

        /**
         * @return The time in seconds of the last bar start before recording started
         */
        double getLastBarStartSeconds() const
        {
            const double qn_duration_sec = 60.0 / bpm;
            const double bar_duration = timeSignatureNum * qn_duration_sec * 4.0 / timeSignatureDenom;
            return refPositionSeconds - std::ceil(refPositionSeconds / bar_duration) * bar_duration;
        }

        /**
         * @return The start recording position in quarter notes
         */
        double getLastBarStartPPQ() const
        {
            const double bar_duration_qn = timeSignatureNum * 4.0 / timeSignatureDenom;
            return refPositionPpq - std::ceil(refPositionPpq / bar_duration_qn) * bar_duration_qn;
        }
    };

    explicit TimeQuantizeOptions(NeuralNoteAudioProcessor* inProcessor);

    ~TimeQuantizeOptions() override;

    void prepareToPlay(double inSampleRate);

    void processBlock(int inNumSamples);

    void fileLoaded();

    void setParameters(bool inEnable, TimeQuantizeUtils::TimeDivisions inDivision, float inQuantizationForce);

    std::vector<Notes::Event> quantize(const std::vector<Notes::Event>& inNoteEvents) const;

    void clear();

    void saveStateToValueTree(bool inSetExportTempo);

    bool checkInfoUpdated();

    TimeQuantizeInfo getTimeQuantizeInfo() const;

private:
    void _setInfo(const Optional<AudioPlayHead::PositionInfo>& inPositionInfoPtr);

    static double _quantizeTime(
        double inEventTime, double inBPM, double inTimeDivision, double inStartTimeQN, float inQuantizationForce);

    static bool isPlayheadPlaying(const Optional<AudioPlayHead::PositionInfo>& inPositionInfoPtr);

    void valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property) override;

    NeuralNoteAudioProcessor* mProcessor;

    CriticalSection mInfoCriticalSection;
    TimeQuantizeInfo mTimeQuantizeInfo;
    Parameters mParameters;

    bool mWasRecording = false;
    bool mWasPlaying = false;
    std::atomic<bool> mInfoUpdated = false;

    int64 mNumRecordedSamples = 0;
    double mSampleRate = 44100.0;
};

#endif // RhythmOptions_h
