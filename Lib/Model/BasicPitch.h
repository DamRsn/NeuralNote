//
// Created by Damien Ronssin on 10.03.23.
//

#ifndef BasicPitch_h
#define BasicPitch_h

#include "BasicPitchCNN.h"
#include "Constants.h"
#include "Features.h"
#include "Notes.h"

/**
 * Class to get midi transcription from raw audio.
 */
class BasicPitch
{
public:
    BasicPitch() = default;

    /**
     * Resets all states of model, clear the posteriorgrams vector computed by the CNN and the note event vector.
     */
    void reset();

    /**
     * Set parameters for next transcription or midi update.
     * @param inNoteSensibility Note sensibility threshold (0.05, 0.95). Higher gives more notes.
     * @param inSplitSensibility Split sensibility threshold (0.05, 0.95). Higher will split note more, lower will merge close notes with same pitch
     * @param inMinNoteDurationMs Minimum note duration to keep in ms.
     */
    void setParameters(float inNoteSensibility,
                       float inSplitSensibility,
                       float inMinNoteDurationMs);

    /**
     * Transcribe the input audio. The note event vector can be obtained after this with getNoteEvents
     * @param inAudio Pointer to raw audio (must be at 22050 Hz)
     * @param inNumSamples Number of input samples available.
     */
    void transcribeToMIDI(float* inAudio, int inNumSamples);

    /**
     * Function to call to update the midi transcription with new parameters.
     * The whole Features + CNN is not rerun for this. Only Notes::Convert is.
     */
    void updateMIDI();

    /**
     * @return Note event vector.
     */
    const std::vector<Notes::Event>& getNoteEvents() const;

private:
    // Posteriorgrams vector
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
