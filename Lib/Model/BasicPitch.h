//
// Created by Damien Ronssin on 10.03.23.
//

#ifndef BasicPitch_h
#define BasicPitch_h

#include "BasicPitchCNN.h"
#include "Constants.h"
#include "Features.h"
#include "MidiFileWriter.h"
#include "Notes.h"

class BasicPitch
{
public:
    BasicPitch();

    void reset();

    void setParameters(float inNoteSensibility,
                       float inSplitSensibility,
                       float inMinNoteDurationMs,
                       int inPitchBendMode);

    void transcribeToMIDI(float* inAudio, int inNumSamples);

    void updateMIDI();

    const std::vector<Notes::Event>& getNoteEvents() const;

private:
    std::vector<std::vector<float>> mContoursPG;
    std::vector<std::vector<float>> mNotesPG;
    std::vector<std::vector<float>> mOnsetsPG;

    std::vector<Notes::Event> mNoteEvents;

    Notes::ConvertParams mParams;

    size_t mNumFrames = 0;

    Features mFeaturesCalculator;
    BasicPitchCNN mBasicPitchCNN;
    Notes mNotesCreator;
};

#endif // BasicPitch_h
