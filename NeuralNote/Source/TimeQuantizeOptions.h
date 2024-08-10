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

        static double qnToSec(double inDurationQn, double inBPM) { return inDurationQn * 60.0 / inBPM; }

        static double secToQn(double inDurationSeconds, double inBPM) { return inDurationSeconds * inBPM / 60.0; }

        /**
         * @return The start position in quarter notes
         */
        double getStartQn() const { return refPositionQn - secToQn(refPositionSeconds, bpm); }

        /**
         * @return
         */
        double getStartLastBarQn() const
        {
            const double bar_duration_qn = timeSignatureNum * 4.0 / timeSignatureDenom;
            const auto start_qn = getStartQn();

            double new_pos_qn = refLastBarQn;

            // TODO: don't use while loop
            while (new_pos_qn > start_qn) {
                new_pos_qn -= bar_duration_qn;
            }

            // // Without while loop:
            // auto new_pos_qn_2 = std::fmod(refLastBarQn, bar_duration_qn);
            //
            // if (new_pos_qn_2 > start_qn) {
            //     new_pos_qn_2 -= bar_duration_qn;
            // }
            //
            // jassert(std::abs(new_pos_qn_2 - new_pos_qn) < 1e-6);

            double num_bars = std::ceil((refLastBarQn - start_qn) / bar_duration_qn);
            auto start_last_bar = refLastBarQn - num_bars * bar_duration_qn;

            jassert(std::abs(start_last_bar - new_pos_qn) < 1e-6);

            return start_last_bar;
        }

        /**
         * @return The time in seconds of the last bar start before recording started. Will be <= 0.
         */
        double getStartLastBarSec() const
        {
            const double bar_duration_qn = timeSignatureNum * 4.0 / timeSignatureDenom;
            const double bar_duration_sec = qnToSec(bar_duration_qn, bpm);
            const double ref_last_bar_seconds = refPositionSeconds - qnToSec(refPositionQn - refLastBarQn, bpm);

            double first_start_bar_sec = ref_last_bar_seconds;

            // TODO: don't use while loop
            while (first_start_bar_sec > 0) {
                first_start_bar_sec -= bar_duration_sec;
            }

            // TODO: what if last bar_seconds is negative
            // Without while loop:
            auto first_start_bar_sec_2 = std::fmod(ref_last_bar_seconds, bar_duration_sec);

            if (first_start_bar_sec_2 > 0) {
                first_start_bar_sec_2 -= bar_duration_sec;
            }

            jassert(std::abs(first_start_bar_sec - first_start_bar_sec_2) < 1e-6);

            auto num_bars = static_cast<int>(std::ceil(ref_last_bar_seconds / bar_duration_sec));
            auto first_start_bar_sec_3 = ref_last_bar_seconds - num_bars * bar_duration_sec;
            jassert(std::abs(first_start_bar_sec - first_start_bar_sec_3) < 1e-6);

            return first_start_bar_sec_3;
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
