//
// Created by Damien Ronssin on 05.03.23.
//

#ifndef AUDIO2MIDIPLUGIN_CNN_TEST_H
#define AUDIO2MIDIPLUGIN_CNN_TEST_H

#include "BasicPitchCNN.h"
#include "test_utils.h"
#include "Constants.h"
#include <fstream>

bool cnn_test()
{
    std::ifstream features_python_stream(std::string(TEST_DATA_DIR)
                                         + "/features_onnx.csv");
    auto features_python = test_utils::loadCSVDataFile<float>(features_python_stream);

    std::ifstream contours_python_stream(std::string(TEST_DATA_DIR) + "/contours.csv");
    auto contours_python_tmp = test_utils::loadCSVDataFile<float>(contours_python_stream);
    std::ifstream notes_python_stream(std::string(TEST_DATA_DIR) + "/notes.csv");
    auto notes_python_tmp = test_utils::loadCSVDataFile<float>(notes_python_stream);
    std::ifstream onsets_python_stream(std::string(TEST_DATA_DIR) + "/onsets.csv");
    auto onsets_python_tmp = test_utils::loadCSVDataFile<float>(onsets_python_stream);

    size_t num_frames_python = features_python.size() / (NUM_HARMONICS * NUM_FREQ_IN);

    std::vector<std::vector<float>> contours_python(num_frames_python,
                                                    std::vector<float>(NUM_FREQ_IN));
    std::vector<std::vector<float>> notes_python(num_frames_python,
                                                 std::vector<float>(NUM_FREQ_OUT));
    std::vector<std::vector<float>> onsets_python(num_frames_python,
                                                  std::vector<float>(NUM_FREQ_OUT));

    for (size_t i = 0; i < num_frames_python; i++)
    {
        std::copy(contours_python_tmp.begin() + i * NUM_FREQ_IN,
                  contours_python_tmp.begin() + (i + 1) * NUM_FREQ_IN,
                  contours_python[i].begin());

        std::copy(notes_python_tmp.begin() + i * NUM_FREQ_OUT,
                  notes_python_tmp.begin() + (i + 1) * NUM_FREQ_OUT,
                  notes_python[i].begin());

        std::copy(onsets_python_tmp.begin() + i * NUM_FREQ_OUT,
                  onsets_python_tmp.begin() + (i + 1) * NUM_FREQ_OUT,
                  onsets_python[i].begin());
    }

    std::vector<float> in_frame(NUM_HARMONICS * NUM_FREQ_IN, 0.0f);
    std::vector<std::vector<float>> contours(num_frames_python,
                                             std::vector<float>(NUM_FREQ_IN));
    std::vector<std::vector<float>> notes(num_frames_python,
                                          std::vector<float>(NUM_FREQ_OUT));
    std::vector<std::vector<float>> onsets(num_frames_python,
                                           std::vector<float>(NUM_FREQ_OUT));

    BasicPitchCNN cnn;

    cnn.reset();

    // Inference
    for (size_t i = 0; i < num_frames_python; i++)
    {
        std::copy(features_python.begin() + i * NUM_HARMONICS * NUM_FREQ_IN,
                  features_python.begin() + (i + 1) * NUM_HARMONICS * NUM_FREQ_IN,
                  in_frame.begin());

        cnn.frameInference(in_frame, contours[i], notes[i], onsets[i]);
    }

    const float threshold = 1e-6f;
    auto lookahead = static_cast<size_t>(cnn.getNumFramesLookahead());

    // Contours
    int num_err_contours = 0;
    float max_err_contours = 0;

    for (size_t n = lookahead; n < num_frames_python - lookahead; n++)
    {
        for (size_t i = 0; i < NUM_FREQ_IN; i++)
        {
            float err = std::abs(contours[n + lookahead][i] - contours_python[n][i]);

            max_err_contours = std::max(max_err_contours, err);

            if (err > threshold)
            {
                num_err_contours += 1;
            }
        }
    }

    std::cout << "Contours test: num errors = " << num_err_contours << " over "
              << (num_frames_python - 2 * lookahead) * NUM_FREQ_IN << " values."
              << std::endl;
    std::cout << "Max contours error is " << max_err_contours << std::endl;

    // Notes
    int num_err_notes = 0;
    float max_err_notes = 0;

    for (size_t n = lookahead; n < num_frames_python - lookahead; n++)
    {
        for (size_t i = 0; i < NUM_FREQ_OUT; i++)
        {
            float err = std::abs(notes[n + lookahead][i] - notes_python[n][i]);

            max_err_notes = std::max(max_err_notes, err);

            if (err > threshold)
            {
                num_err_notes += 1;
            }
        }
    }

    std::cout << "Notes test: num errors = " << num_err_notes << " over "
              << (num_frames_python - 2 * lookahead) * NUM_FREQ_OUT << " values."
              << std::endl;
    std::cout << "Max note error is " << max_err_notes << std::endl;

    // Onsets
    int num_err_onsets = 0;
    float max_err_onsets = 0;

    for (size_t n = lookahead; n < num_frames_python - lookahead; n++)
    {
        for (size_t i = 0; i < NUM_FREQ_OUT; i++)
        {
            float err = std::abs(onsets[n + lookahead][i] - onsets_python[n][i]);

            max_err_onsets = std::max(max_err_onsets, err);

            if (err > threshold)
                num_err_onsets += 1;
        }
    }

    std::cout << "Onset test: num errors = " << num_err_onsets << " over "
              << (num_frames_python - 2 * lookahead) * NUM_FREQ_OUT << " values."
              << std::endl;
    std::cout << "Max onset error is " << max_err_onsets << std::endl;

    return (num_err_contours + num_err_notes + num_err_onsets) == 0;
}

#endif //AUDIO2MIDIPLUGIN_CNN_TEST_H
