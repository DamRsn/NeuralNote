//
// Created by Damien Ronssin on 10.03.23.
//

#include "BasicPitch.h"

BasicPitch::BasicPitch()
{
}

void BasicPitch::reset()
{
    mBasicPitchCNN.reset();
    mContoursPG.clear();
    mNotesPG.clear();
    mOnsetsPG.clear();

    mNumFrames = 0;
}

void BasicPitch::setParameters()
{
    // TODO:
}

std::vector<Notes::Event> BasicPitch::transribeToMIDI(float* inAudio, int inNumSamples)
{
    const float* stacked_cqt =
        mFeaturesCalculator.computeFeatures(inAudio, inNumSamples, mNumFrames);

    mOnsetsPG.resize(mNumFrames, std::vector<float>(NUM_FREQ_OUT, 0.0f));
    mNotesPG.resize(mNumFrames, std::vector<float>(NUM_FREQ_OUT, 0.0f));
    mContoursPG.resize(mNumFrames, std::vector<float>(NUM_FREQ_IN, 0.0f));

    mBasicPitchCNN.reset();

    // TODO: align everything with CNN lookahead
    for (size_t frame_idx = 0; frame_idx < mNumFrames; frame_idx++)
    {
        mBasicPitchCNN.frameInference(stacked_cqt
                                          + frame_idx * NUM_HARMONICS * NUM_FREQ_IN,
                                      mContoursPG[frame_idx],
                                      mNotesPG[frame_idx],
                                      mOnsetsPG[frame_idx]);
    }

    std::vector<Notes::Event> note_events =
        mNotesCreator.convert(mNotesPG, mOnsetsPG, mContoursPG, mParams);

    return note_events;
}

std::vector<Notes::Event> BasicPitch::updateMIDI()
{
    return std::vector<Notes::Event>();
}
