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

void BasicPitchCNN::reset()
{
    for (auto& array: mContoursCircularBuffer)
    {
        array.fill(0.0f);
    }

    for (auto& array: mNotesCircularBuffer)
    {
        array.fill(0.0f);
    }

    for (auto& array: mConcat2CircularBuffer)
    {
        array.fill(0.0f);
    }

    mCNNContour.reset();
    mCNNNote.reset();
    mCNNOnsetInput.reset();
    mCNNOnsetOutput.reset();

    mNoteIdx = 0;
    mContourIdx = 0;
    mConcat2Idx = 0;

    mInputArray.fill(0.0f);
}

int BasicPitchCNN::getNumFramesLookahead()
{
    return mTotalLookahead;
}

void BasicPitchCNN::frameInference(const float* inData,
                                   std::vector<float>& outContours,
                                   std::vector<float>& outNotes,
                                   std::vector<float>& outOnsets)
{
    // Checks on parameters
    assert(outContours.size() == NUM_FREQ_IN);
    assert(outNotes.size() == NUM_FREQ_OUT);
    assert(outOnsets.size() == NUM_FREQ_OUT);

    // Copy data in aligned input array for inference
    std::copy(inData, inData + NUM_HARMONICS * NUM_FREQ_IN, mInputArray.begin());

    _runModels();

    // Fill output vectors
    std::copy(mCNNOnsetOutput.getOutputs(),
              mCNNOnsetOutput.getOutputs() + NUM_FREQ_OUT,
              outOnsets.begin());

    std::copy(
        mNotesCircularBuffer[(size_t) _wrapIndex(mNoteIdx + 1, mNumNoteStored)].begin(),
        mNotesCircularBuffer[(size_t) _wrapIndex(mNoteIdx + 1, mNumNoteStored)].end(),
        outNotes.begin());

    std::copy(
        mContoursCircularBuffer[(size_t) _wrapIndex(mContourIdx + 1, mNumContourStored)]
            .begin(),
        mContoursCircularBuffer[(size_t) _wrapIndex(mContourIdx + 1, mNumContourStored)]
            .end(),
        outContours.begin());

    // Increment index for different circular buffers
    mContourIdx = (mContourIdx == mNumContourStored - 1) ? 0 : mContourIdx + 1;
    mNoteIdx = (mNoteIdx == mNumNoteStored - 1) ? 0 : mNoteIdx + 1;
    mConcat2Idx = (mConcat2Idx == mNumConcat2Stored - 1) ? 0 : mConcat2Idx + 1;
}

void BasicPitchCNN::_runModels()
{
    // Run models and push results in appropriate circular buffer
    mCNNOnsetInput.forward(mInputArray.data());
    std::copy(mCNNOnsetInput.getOutputs(),
              mCNNOnsetInput.getOutputs() + 32 * NUM_FREQ_OUT,
              mConcat2CircularBuffer[(size_t) mConcat2Idx].begin());

    mCNNContour.forward(mInputArray.data());
    std::copy(mCNNContour.getOutputs(),
              mCNNContour.getOutputs() + NUM_FREQ_IN,
              mContoursCircularBuffer[(size_t) mContourIdx].begin());

    mCNNNote.forward(mCNNContour.getOutputs());
    std::copy(mCNNNote.getOutputs(),
              mCNNNote.getOutputs() + NUM_FREQ_OUT,
              mNotesCircularBuffer[(size_t) mNoteIdx].begin());

    // Concat operation with correct frame shift
    _concat();

    mCNNOnsetOutput.forward(mConcatArray.data());
}

constexpr int BasicPitchCNN::_wrapIndex(int inIndex, int inSize)
{
    int wrapped_index = inIndex % inSize;

    if (wrapped_index < 0)
    {
        wrapped_index += inSize;
    }

    return wrapped_index;
}

void BasicPitchCNN::_concat()
{
    auto concat2_index = (size_t) _wrapIndex(mConcat2Idx + 1, mNumConcat2Stored);

    for (size_t i = 0; i < NUM_FREQ_OUT; i++)
    {
        mConcatArray[i * 33] = mCNNNote.getOutputs()[i];
        std::copy(mConcat2CircularBuffer[concat2_index].begin() + i * 32,
                  mConcat2CircularBuffer[concat2_index].begin() + (i + 1) * 32,
                  mConcatArray.begin() + i * 33 + 1);
    }
}
