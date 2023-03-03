//
// Created by Damien Ronssin on 03.03.23.
//

#ifndef BasicPitchCNN_h
#define BasicPitchCNN_h

#include "RTNeural/RTNeural.h"

class BasicPitchCNN
{
public:
private:
    static constexpr int NUM_HARMONICS = 8;
    static constexpr int NUM_FREQ_IN = 264;
    static constexpr int NUM_FREQ_OUT = 88;

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
                     RTNeural::ReLuActivationT<float, 32 * NUM_FREQ_IN>,
                     RTNeural::Conv2DT<float, 32, 1, NUM_FREQ_OUT, 7, 3, 1, 1, false>,
                     RTNeural::SigmoidActivationT<float, NUM_FREQ_OUT>>
        mCNNNote;

    RTNeural::ModelT<float,
                     NUM_FREQ_IN * NUM_HARMONICS,
                     32 * NUM_FREQ_OUT,
                     RTNeural::Conv2DT<float, 1, 32, NUM_FREQ_IN, 5, 5, 1, 3, false>,
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
