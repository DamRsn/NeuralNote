//
// Created by Damien Ronssin on 05.03.23.
//

#ifndef NN_PERF_TEST_H
#define NN_PERF_TEST_H

#include "Constants.h"
#include "Features.h"
#include "BasicPitchCNN.h"
#include "test_utils.h"

#include <fstream>
#include <chrono>

bool perf_test()
{
    std::ifstream input_audio_stream(std::string(TEST_DATA_DIR) + "/input_audio.csv");
    auto audio = test_utils::loadCSVDataFile<float>(input_audio_stream);

    Features feature_calculator;

    size_t num_out_frames;

    BasicPitchCNN cnn;

    auto start_time = std::chrono::high_resolution_clock::now();
    const float* stacked_cqt_data =
        feature_calculator.computeFeatures(audio.data(), audio.size(), num_out_frames);

    auto stop_time_onnx = std::chrono::high_resolution_clock::now();

    std::vector<std::vector<float>> contours(num_out_frames,
                                             std::vector<float>(NUM_FREQ_IN));
    std::vector<std::vector<float>> notes(num_out_frames,
                                          std::vector<float>(NUM_FREQ_OUT));
    std::vector<std::vector<float>> onsets(num_out_frames,
                                           std::vector<float>(NUM_FREQ_OUT));

    for (size_t i = 0; i < num_out_frames; i++)
    {
        cnn.frameInference(stacked_cqt_data + i * NUM_HARMONICS * NUM_FREQ_IN,
                           contours[i],
                           notes[i],
                           onsets[i]);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> execution_duration = end_time - start_time;
    std::chrono::duration<double> execution_duration_onnx = stop_time_onnx - start_time;
    std::chrono::duration<double> execution_duration_cnn = end_time - stop_time_onnx;

    std::cout << "Audio to process " << audio.size() / 22050 << " seconds:" << std::endl;
    std::cout << "Total Execution time: " << execution_duration.count() << " seconds"
              << std::endl;
    std::cout << "Execution time Features (ONNX): " << execution_duration_onnx.count()
              << " seconds" << std::endl;
    std::cout << "Execution time CNN (RTNeural): " << execution_duration_cnn.count()
              << " seconds" << std::endl;

    std::cout << "Success" << std::endl;

    return true;
}

#endif //NN_PERF_TEST_H
