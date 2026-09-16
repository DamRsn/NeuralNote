//
// Created by Damien Ronssin on 10.06.23.
//

#include "SynthController.h"
#include "PluginProcessor.h"

SynthController::SynthController(NeuralNoteAudioProcessor* inProcessor)
    : mProcessor(inProcessor)
{
    // Sized for the worst block: every active note released and a fresh set started, none of which
    // may allocate on the audio thread. Generous on purpose -- a few KB against a resize in the
    // callback is not a trade worth thinking about twice.
    mMidiBuffer.ensureSize(16 * (2 * NoteScheduler::MAX_ACTIVE_NOTES + 128));
}

void SynthController::setSampleRate(double inSampleRate)
{
    mScheduler.setSampleRate(inSampleRate);
}

void SynthController::setNotes(std::vector<NoteEvent>& ioNotes)
{
    const ScopedLock sl(mProcessor->getCallbackLock());

    mScheduler.setNotes(ioNotes);
}

const MidiBuffer& SynthController::generateNextMidiBuffer(int inNumSamples, bool inIsPlaying)
{
    mMidiBuffer.clear();

    mScheduler.renderNextBlock(mMidiBuffer, inNumSamples, inIsPlaying);

    // Only while playing: this is called every block now, and with no audio loaded the duration is
    // zero, so an unguarded check would rewind the transport on every callback forever.
    if (inIsPlaying && mScheduler.getTimeSeconds() >= mProcessor->getSourceAudioManager()->getAudioSampleDuration()) {
        // Stop playing and reset to start
        mProcessor->getPlayer()->setPlayingState(false);
        setNewTimeSeconds(0);
    }

    return mMidiBuffer;
}

void SynthController::stopAllNotes()
{
    mScheduler.stopAllNotes();
}

void SynthController::emitActiveNotesOffTo(MidiBuffer& outMidiBuffer) const
{
    mScheduler.emitActiveNotesOffTo(outMidiBuffer);
}

void SynthController::reset()
{
    const ScopedLock sl(mProcessor->getCallbackLock());

    mScheduler.reset();
}

void SynthController::setNewTimeSeconds(double inNewTime)
{
    mScheduler.setTimeSeconds(inNewTime);
}

double SynthController::getCurrentTimeSeconds() const
{
    return mScheduler.getTimeSeconds();
}
