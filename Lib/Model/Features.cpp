//
// Created by Damien Ronssin on 04.03.23.
//

#include "Features.h"

Features::Features()
    : mMemoryInfo(nullptr)
    , mSession(nullptr)
{
    mMemoryInfo = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);

    mSessionOptions.SetInterOpNumThreads(1);
    mSessionOptions.SetIntraOpNumThreads(1);

    mSession = Ort::Session(mEnv,
                            BinaryData::features_model_ort,
                            BinaryData::features_model_ortSize,
                            mSessionOptions);
}

const float*
    Features::computeFeatures(float* inAudio, size_t inNumSamples, size_t& outNumFrames)
{
    mInputShape[0] = 1;
    mInputShape[1] = static_cast<int64_t>(inNumSamples);
    mInputShape[2] = 1;

    mInput.clear();
    mInput.push_back(Ort::Value::CreateTensor<float>(
        mMemoryInfo, inAudio, inNumSamples, mInputShape.data(), mInputShape.size()));

    mOutput = mSession.Run(mRunOptions, mInputNames, mInput.data(), 1, mOutputNames, 1);

    auto out_shape = mOutput[0].GetTensorTypeAndShapeInfo().GetShape();
    assert(out_shape[0] == 1 && out_shape[2] == NUM_FREQ_IN
           && out_shape[3] == NUM_HARMONICS);

    outNumFrames = static_cast<size_t>(out_shape[1]);

    mInput.clear();

    return mOutput[0].GetTensorData<float>();
}
