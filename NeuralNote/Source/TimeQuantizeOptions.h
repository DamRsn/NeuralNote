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
        double refLastBarQn = 0.0;
        double refPositionQn = 0.0;
        double refPositionSeconds = 0.0;

        /**
         * Convert a duration in quarter notes to seconds
         */
        static double qnToSec(double inDurationQn, double inBPM) { return inDurationQn * 60.0 / inBPM; }

        /**
         * Convert a duration in seconds to quarter notes
         */
        static double secToQn(double inDurationSeconds, double inBPM) { return inDurationSeconds * inBPM / 60.0; }

        /**
         * @return The start position in quarter notes
         */
        double getStartQn() const { return refPositionQn - secToQn(refPositionSeconds, bpm); }

        /**
         * @return Last bar position (before recording started) in quarter notes
         */
        double getStartLastBarQn() const
        {
            const double bar_duration_qn = timeSignatureNum * 4.0 / timeSignatureDenom;
            const auto start_qn = getStartQn();

            double num_bars = std::ceil((refLastBarQn - start_qn) / bar_duration_qn);

            return refLastBarQn - num_bars * bar_duration_qn;
        }

        /**
         * @return Get the time in seconds for the last bar start before recording started. Will be <= 0.
         */
        double getStartLastBarSec() const
        {
            const double bar_duration_qn = timeSignatureNum * 4.0 / timeSignatureDenom;
            const double bar_duration_sec = qnToSec(bar_duration_qn, bpm);
            const double ref_last_bar_seconds = refPositionSeconds - qnToSec(refPositionQn - refLastBarQn, bpm);

            const auto num_bars = static_cast<int>(std::ceil(ref_last_bar_seconds / bar_duration_sec));

            return ref_last_bar_seconds - num_bars * bar_duration_sec;
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
    int mNumPlayingProcessBlock = 0;
    static constexpr int mNumPlayingProcessBlockBeforeSetInfo = 2;

    // To signal that the info has been updated for the timer on main thread
    std::atomic<bool> mInfoUpdated = false;

    int64 mNumRecordedSamples = 0;
    double mSampleRate = 44100.0;
};

#endif // RhythmOptions_h
