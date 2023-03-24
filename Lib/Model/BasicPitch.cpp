//
// Created by Damien Ronssin on 10.03.23.
//

#include "BasicPitch.h"

void BasicPitch::reset()
{
    mBasicPitchCNN.reset();
    mContoursPG.clear();
    mNotesPG.clear();
    mOnsetsPG.clear();
    mNoteEvents.clear();

    mNumFrames = 0;
}

void BasicPitch::setParameters(float inNoteSensibility,
                               float inSplitSensibility,
                               float inMinNoteDurationMs)
{
    mParams.frameThreshold = 1.0f - inNoteSensibility;
    mParams.onsetThreshold = 1.0f - inSplitSensibility;

    mParams.minNoteLength = static_cast<int>(
        std::round(inMinNoteDurationMs * FFT_HOP / BASIC_PITCH_SAMPLE_RATE));

    mParams.pitchBend = MultiPitchBend;
    mParams.melodiaTrick = true;
    mParams.inferOnsets = true;
}

void BasicPitch::transcribeToMIDI(float* inAudio, int inNumSamples)
{
    // To test if downsampling works as expected
#if SAVE_DOWNSAMPLED_AUDIO
    auto file = juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                    .getChildFile("Test_Downsampled.wav");

    std::unique_ptr<AudioFormatWriter> format_writer;

    format_writer.reset(WavAudioFormat().createWriterFor(
        new FileOutputStream(file), 22050, 1, 16, {}, 0));

    if (format_writer != nullptr)
    {
        AudioBuffer<float> tmp_buffer;
        tmp_buffer.setSize(1, inNumSamples);
        tmp_buffer.copyFrom(0, 0, inAudio, inNumSamples);
        format_writer->writeFromAudioSampleBuffer(tmp_buffer, 0, inNumSamples);

        format_writer->flush();

        file.revealToUser();
    }
#endif

    const float* stacked_cqt =
        mFeaturesCalculator.computeFeatures(inAudio, inNumSamples, mNumFrames);

    mOnsetsPG.resize(mNumFrames, std::vector<float>(NUM_FREQ_OUT, 0.0f));
    mNotesPG.resize(mNumFrames, std::vector<float>(NUM_FREQ_OUT, 0.0f));
    mContoursPG.resize(mNumFrames, std::vector<float>(NUM_FREQ_IN, 0.0f));

    mBasicPitchCNN.reset();

    const size_t num_lh_frames = BasicPitchCNN::getNumFramesLookahead();

    std::vector<float> zero_stacked_cqt(NUM_HARMONICS * NUM_FREQ_IN, 0.0f);

    // Run the CNN with 0 input and discard output (only for num_lh_frames)
    for (int i = 0; i < num_lh_frames; i++)
    {
        mBasicPitchCNN.frameInference(
            zero_stacked_cqt.data(), mContoursPG[0], mNotesPG[0], mOnsetsPG[0]);
    }

    // Run the CNN with real inputs and discard outputs (only for num_lh_frames)
    for (size_t frame_idx = 0; frame_idx < num_lh_frames; frame_idx++)
    {
        mBasicPitchCNN.frameInference(stacked_cqt
                                          + frame_idx * NUM_HARMONICS * NUM_FREQ_IN,
                                      mContoursPG[0],
                                      mNotesPG[0],
                                      mOnsetsPG[0]);
    }

    // Run the CNN with real inputs and correct outputs
    for (size_t frame_idx = num_lh_frames; frame_idx < mNumFrames; frame_idx++)
    {
        mBasicPitchCNN.frameInference(stacked_cqt
                                          + frame_idx * NUM_HARMONICS * NUM_FREQ_IN,
                                      mContoursPG[frame_idx - num_lh_frames],
                                      mNotesPG[frame_idx - num_lh_frames],
                                      mOnsetsPG[frame_idx - num_lh_frames]);
    }

    // Run end with zeroes as input and last frames as output
    for (size_t frame_idx = mNumFrames; frame_idx < mNumFrames + num_lh_frames;
         frame_idx++)
    {
        mBasicPitchCNN.frameInference(zero_stacked_cqt.data(),
                                      mContoursPG[frame_idx - num_lh_frames],
                                      mNotesPG[frame_idx - num_lh_frames],
                                      mOnsetsPG[frame_idx - num_lh_frames]);
    }

    mNoteEvents = mNotesCreator.convert(mNotesPG, mOnsetsPG, mContoursPG, mParams);
}

void BasicPitch::updateMIDI()
{
    mNoteEvents = mNotesCreator.convert(mNotesPG, mOnsetsPG, mContoursPG, mParams);
}

const std::vector<Notes::Event>& BasicPitch::getNoteEvents() const
{
    return mNoteEvents;
}
