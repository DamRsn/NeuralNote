//
// Created by Damien Ronssin on 05.03.23.
//

#ifndef NN_FEATURES_TEST_H
#define NN_FEATURES_TEST_H

#include "Features.h"
#include "test_utils.h"
#include "Constants.h"
#include <fstream>

bool feature_test()
{
    std::ifstream input_audio_stream(std::string(TEST_DATA_DIR) + "/input_audio.csv");
    auto audio = test_utils::loadCSVDataFile<float>(input_audio_stream);

    std::ifstream features_python_stream(std::string(TEST_DATA_DIR)
                                         + "/features_onnx.csv");
    auto features_python = test_utils::loadCSVDataFile<float>(features_python_stream);

    size_t num_out_frames_python = features_python.size() / (NUM_HARMONICS * NUM_FREQ_IN);

    // Compute features from audio.
    size_t num_out_frames;
    Features feature_calculator;
    const float* stacked_cqt_data =
        feature_calculator.computeFeatures(audio.data(), audio.size(), num_out_frames);

    if (num_out_frames != num_out_frames_python)
    {
        std::cout << "Different number of frames in C++ and Python." << std::endl;
        return false;
    }

    int num_err = 0;
    float max_err = 0;
    float threshold = 1e-3f;

    for (size_t i = 0; i < features_python.size(); i++)
    {
        float err = std::abs(features_python[i] - stacked_cqt_data[i]);

        max_err = std::max(max_err, err);

        if (err > threshold)
            num_err += 1;
    }

    std::cout << "Features test: num errors = " << num_err << " over "
              << features_python.size() << " values." << std::endl;
    std::cout << "Max error is " << max_err << std::endl;

    return num_err == 0;
}

#endif //NN_FEATURES_TEST_H
