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
}

int BasicPitchCNN::getNumFramesLookahead() const
{
    return mTotalLookahead;
}

void BasicPitchCNN::frameInference(const std::vector<float>& inData,
                                   std::vector<float>& outContours,
                                   std::vector<float>& outNotes,
                                   std::vector<float>& outOnsets)
{
    // Checks on parameters
    jassert(inData.size() == NUM_HARMONICS * NUM_FREQ_IN);
    jassert(outContours.size() == NUM_FREQ_IN);
    jassert(outNotes.size() == NUM_FREQ_OUT);
    jassert(outOnsets.size() == NUM_FREQ_OUT);

    // Copy data in aligned input array for inference
    std::copy(inData.begin(), inData.end(), mInputArray.begin());

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
    std::copy(mNotesCircularBuffer[(size_t) mNoteIdx].begin(),
              mNotesCircularBuffer[(size_t) mNoteIdx].end(),
              mConcatArray.begin());

    std::copy(mConcat2CircularBuffer[(size_t) _wrapIndex(
                                         mConcat2Idx
                                             - (mNumConcat2Stored
                                                - mNumNoteStored), // + 1 should be equiv
                                         mNumConcat2Stored)]
                  .begin(),
              mConcat2CircularBuffer[(size_t) _wrapIndex(
                                         mConcat2Idx
                                             - (mNumConcat2Stored
                                                - mNumNoteStored), // + 1 should be equiv
                                         mNumConcat2Stored)]
                  .end(),
              mConcatArray.begin() + NUM_FREQ_OUT);

    mCNNOnsetOutput.forward(mConcatArray.data());

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
    mNoteIdx = (mNoteIdx== mNumNoteStored - 1) ? 0 : mNoteIdx + 1;
    mConcat2Idx = (mConcat2Idx == mNumConcat2Stored - 1) ? 0 : mConcat2Idx + 1;
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
