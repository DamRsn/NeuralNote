//
// Created by Damien Ronssin on 03.03.23.
//

#include "BasicPitchCNN.h"

using json = nlohmann::json;

BasicPitchCNN::BasicPitchCNN()
{
    json json_cnn_contour = json::parse(BinaryData::cnn_contour_model_json,
                                        BinaryData::cnn_contour_model_json
                                            + BinaryData::cnn_contour_model_jsonSize);

    mCNNContour.parseJson(json_cnn_contour);

    json json_cnn_note = json::parse(BinaryData::cnn_note_model_json,
                                     BinaryData::cnn_note_model_json
                                         + BinaryData::cnn_note_model_jsonSize);

    mCNNNote.parseJson(json_cnn_note);

    json json_cnn_onset_input = json::parse(BinaryData::cnn_onset_1_model_json,
                                            BinaryData::cnn_onset_1_model_json
                                                + BinaryData::cnn_onset_1_model_jsonSize);

    mCNNOnsetInput.parseJson(json_cnn_onset_input);

    json json_cnn_onset_output = json::parse(
        BinaryData::cnn_onset_2_model_json,
        BinaryData::cnn_onset_2_model_json + BinaryData::cnn_onset_2_model_jsonSize);

    mCNNOnsetOutput.parseJson(json_cnn_onset_output);
}

BasicPitchCNN::~BasicPitchCNN()
{
}

void BasicPitchCNN::reset()
{
}

int BasicPitchCNN::getNumFramesLookahead() const
{
    return 0;
}

void BasicPitchCNN::frameInference(std::vector<float>& inputData,
                                   std::vector<float>& outContours,
                                   std::vector<float>& outNotes,
                                   std::vector<float>& outOnsets)
{
    ignoreUnused(inputData);
    ignoreUnused(outContours);
    ignoreUnused(outNotes);
    ignoreUnused(outOnsets);
    ignoreUnused(mInputArray);
    ignoreUnused(mCurrentFrameIndex);
}
