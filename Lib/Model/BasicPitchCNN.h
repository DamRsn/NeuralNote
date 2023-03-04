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

    static constexpr int getNumFramesLookahead();

    void frameInference(const std::vector<float>& inData,
                        std::vector<float>& outContours,
                        std::vector<float>& outNotes,
                        std::vector<float>& outOnsets);

private:
    static constexpr int _wrapIndex(int inIndex, int inSize);

    alignas(RTNEURAL_DEFAULT_ALIGNMENT)
        std::array<float, NUM_FREQ_IN * NUM_HARMONICS> mInputArray;

    alignas(RTNEURAL_DEFAULT_ALIGNMENT) std::array<float, 33 * NUM_FREQ_OUT> mConcatArray;

    static constexpr int mLookaheadCNNContour = 3;
    static constexpr int mLookaheadCNNNote = 6;
    static constexpr int mLookaheadCNNOnsetInput = 2;
    static constexpr int mLookaheadCNNOnsetOutput = 1;
    static constexpr int mTotalLookahead =
        mLookaheadCNNContour + mLookaheadCNNNote + mLookaheadCNNOnsetOutput;

    static constexpr int mNumContourStored = mTotalLookahead - mLookaheadCNNContour + 1;
    static constexpr int mNumNoteStored =
        mTotalLookahead - (mLookaheadCNNContour + mLookaheadCNNNote) + 1;
    static constexpr int mNumConcat2Stored =
        mLookaheadCNNContour + mLookaheadCNNNote - mLookaheadCNNOnsetInput + 1;

    std::array<std::array<float, NUM_FREQ_OUT>, mNumContourStored>
        mContoursCircularBuffer;
    std::array<std::array<float, NUM_FREQ_OUT>, mNumNoteStored>
        mNotesCircularBuffer; // Also concat 1
    std::array<std::array<float, 32 * NUM_FREQ_OUT>, mNumConcat2Stored>
        mConcat2CircularBuffer;

    int mContourIdx = 0;
    int mNoteIdx = 0;
    int mConcat2Idx = 0;

    RTNeural::ModelT<
        float,
        NUM_FREQ_IN * NUM_HARMONICS,
        NUM_FREQ_IN,
        RTNeural::Conv2DT<float, NUM_HARMONICS, 8, NUM_FREQ_IN, 3, 39, 1, 1, false>,
        RTNeural::ReLuActivationT<float, 8 * NUM_FREQ_IN>,
        RTNeural::Conv2DT<float, 8, 1, NUM_FREQ_IN, 5, 5, 1, 1, false>,
        RTNeural::SigmoidActivationT<float, NUM_FREQ_IN>>
        mCNNContour;

    RTNeural::ModelT<float,
                     NUM_FREQ_IN,
                     NUM_FREQ_OUT,
                     RTNeural::Conv2DT<float, 1, 32, NUM_FREQ_IN, 7, 7, 1, 3, false>,
                     RTNeural::ReLuActivationT<float, 32 * NUM_FREQ_OUT>,
                     RTNeural::Conv2DT<float, 32, 1, NUM_FREQ_OUT, 7, 3, 1, 1, false>,
                     RTNeural::SigmoidActivationT<float, NUM_FREQ_OUT>>
        mCNNNote;

    RTNeural::ModelT<float,
                     NUM_FREQ_IN * NUM_HARMONICS,
                     32 * NUM_FREQ_OUT,
                     RTNeural::Conv2DT<float, 8, 32, NUM_FREQ_IN, 5, 5, 1, 3, false>,
                     RTNeural::ReLuActivationT<float, 32 * NUM_FREQ_OUT>>
        mCNNOnsetInput;

    RTNeural::ModelT<float,
                     33 * NUM_FREQ_OUT,
                     NUM_FREQ_OUT,
                     RTNeural::Conv2DT<float, 33, 1, NUM_FREQ_OUT, 3, 3, 1, 1, false>,
                     RTNeural::SigmoidActivationT<float, NUM_FREQ_OUT>>
        mCNNOnsetOutput;
};

#endif // BasicPitchCNN_h
