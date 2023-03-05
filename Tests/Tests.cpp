#include <JuceHeader.h>
#include "Features.h"
#include "BasicPitchCNN.h"
#include <vector>
#include "test_utils.h"
#include "features_test.h"
#include "cnn_test.h"

#include <chrono>

int main()
{
    int result = 0;
    std::vector<float> x;

    Features feature_calculator;
    size_t num_input_samples = 10000;
    x.resize(num_input_samples);

    size_t num_out_frames;

    BasicPitchCNN cnn;
    std::vector<float> onsets(NUM_FREQ_OUT, 0.0);
    std::vector<float> notes(NUM_FREQ_OUT, 0.0);
    std::vector<float> contours(NUM_FREQ_IN, 0.0);
    std::vector<float> in_data(NUM_FREQ_IN * NUM_HARMONICS, 0.0);

    auto start_time = std::chrono::high_resolution_clock::now();
    const float* stacked_cqt_data =
        feature_calculator.computeFeatures(x.data(), num_input_samples, num_out_frames);

    auto stop_time_onnx = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < num_out_frames; i++)
    {
        std::copy(stacked_cqt_data + i * NUM_HARMONICS * NUM_FREQ_IN,
                  stacked_cqt_data + (i + 1) * NUM_HARMONICS * NUM_FREQ_IN,
                  in_data.begin());

        cnn.frameInference(in_data, contours, notes, onsets);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> execution_duration = end_time - start_time;
    std::chrono::duration<double> execution_duration_onnx = stop_time_onnx - start_time;
    std::chrono::duration<double> execution_duration_cnn = end_time - stop_time_onnx;

    std::cout << "To process " << num_input_samples / 22050 << " seconds:" << std::endl;
    std::cout << "Total Execution time: " << execution_duration.count() << " seconds"
              << std::endl;
    std::cout << "Execution time Features (ONNX): " << execution_duration_onnx.count()
              << " seconds" << std::endl;
    std::cout << "Execution time CNN (RTNeural): " << execution_duration_cnn.count()
              << " seconds" << std::endl;

    std::cout << "Success" << std::endl;

    std::cout << std::endl << "FEATURE TEST" << std::endl;
    result |= feature_test();

    std::cout << std::endl << "CNN TEST" << std::endl;
    result |= cnn_test();

    return result;
}