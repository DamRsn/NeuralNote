//
// Created by Damien Ronssin on 03.03.23.
//

#ifndef BasicPitchCNN_h
#define BasicPitchCNN_h

#include "Constants.h"
#include "RTNeural/RTNeural.h"
#include "json.hpp"
#include <JuceHeader.h>

class BasicPitchCNN
{
public:
    BasicPitchCNN();

    ~BasicPitchCNN();

    void reset();

    int getNumFramesLookahead() const;

    void frameInference(const std::vector<float>& inData,
                        std::vector<float>& outContours,
                        std::vector<float>& outNotes,
                        std::vector<float>& outOnsets);

private:

    size_t mCurrentFrameIndex = 0;

    alignas(RTNEURAL_DEFAULT_ALIGNMENT)
        std::array<float, NUM_FREQ_IN * NUM_HARMONICS> mInputArray;

    std::array< std::array<float, NUM_FREQ_OUT>, 5> mContoursCircularBuffer;
    std::array< std::array<float, NUM_FREQ_OUT>, 5> mNotesCircularBuffer;
    std::array< std::array<float, NUM_FREQ_OUT>, 5> mOnsetCircularBuffer;

    RTNeural::ModelT<
        float,
        NUM_FREQ_IN * NUM_HARMONICS,
        NUM_FREQ_IN,
        RTNeural::Conv2DT<float, NUM_HARMONICS, 8, NUM_FREQ_IN, 3, 39, 1, 1, false>,
        RTNeural::ReLuActivationT<float, 8 * NUM_FREQ_IN>,
        RTNeural::Conv2DT<float, 8, 1, NUM_FREQ_IN, 5, 5, 1, 1, false>,
        RTNeural::SigmoidActivationT<float, NUM_FREQ_IN>>
        mCNNContour;

    static constexpr int mLookaheadCNNContour = 3;

    RTNeural::ModelT<float,
                     NUM_FREQ_IN,
                     NUM_FREQ_OUT,
                     RTNeural::Conv2DT<float, 1, 32, NUM_FREQ_IN, 7, 7, 1, 3, false>,
                     RTNeural::ReLuActivationT<float, 32 * NUM_FREQ_IN>,
                     RTNeural::Conv2DT<float, 32, 1, NUM_FREQ_OUT, 7, 3, 1, 1, false>,
                     RTNeural::SigmoidActivationT<float, NUM_FREQ_OUT>>
        mCNNNote;

    static constexpr int mLookaheadCNNNote = 6;

    RTNeural::ModelT<float,
                     NUM_FREQ_IN * NUM_HARMONICS,
                     32 * NUM_FREQ_OUT,
                     RTNeural::Conv2DT<float, 1, 32, NUM_FREQ_IN, 5, 5, 1, 3, false>,
                     RTNeural::ReLuActivationT<float, 32 * NUM_FREQ_OUT>>
        mCNNOnsetInput;

    static constexpr int mLookaheadCNNOnsetInput = 2;

    RTNeural::ModelT<float,
                     33 * NUM_FREQ_OUT,
                     NUM_FREQ_OUT,
                     RTNeural::Conv2DT<float, 33, 1, NUM_FREQ_OUT, 3, 3, 1, 1, false>,
                     RTNeural::SigmoidActivationT<float, NUM_FREQ_OUT>>
        mCNNOnsetOutput;

    static constexpr int mLookaheadCNNOnsetOutput = 1;

};

#endif // BasicPitchCNN_h
