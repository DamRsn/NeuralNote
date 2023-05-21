//
// Created by Damien Ronssin on 21.05.23.
//

#include "Synth.h"

Synth::Synth(AudioProcessor* inProcessor)
    : mProcessor(inProcessor)
{
}

void Synth::prepareToPlay(float inSampleRate, int inSamplesPerBlock)
{
    ignoreUnused(inSamplesPerBlock);
    mSampleRate = inSampleRate;

    for (auto& voice: mVoices)
    {
        voice.prepareToPlay(inSampleRate);
    }
}

void Synth::processBlock(AudioBuffer<float>& inAudioBuffer)
{
    for (int i = 0; i < inAudioBuffer.getNumSamples(); i++)
    {
        inAudioBuffer.setSample(0, i, processSample());
    }

    // Copy first channel to eventual other channels
    for (int ch = 1; ch < inAudioBuffer.getNumChannels(); ch++)
    {
        inAudioBuffer.copyFrom(ch, 0, inAudioBuffer, 0, 0, inAudioBuffer.getNumSamples());
    }
}

float Synth::processSample()
{
    float out = 0.0f;
    for (auto& voice: mVoices)
    {
        if (voice.isActive())
        {
            out += voice.getNextSample();
        }
    }

    return out;
}

void Synth::reset()
{
    for (auto& voice: mVoices)
    {
        voice.reset();
    }

    mCurrentEventIndex = 0;
}

void Synth::noteOn(int inMidiNote, float inInitBend, float inAmplitude)
{
    for (auto& voice: mVoices)
    {
        if (!voice.isActive())
        {
            voice.noteOn(inMidiNote, inInitBend, inAmplitude);
            break;
        }
    }
}

void Synth::noteOff(int inMidiNote)
{
    for (auto& voice: mVoices)
    {
        if (voice.isActive() && voice.getCurrentNote() == inMidiNote)
        {
            voice.noteOff();
            break;
        }
    }
}

void Synth::shutAllVoices()
{
    for (auto& voice: mVoices)
    {
        if (voice.isActive())
        {
            voice.noteOff();
        }
    }
}

std::vector<SingleEvent>
    Synth::buildEventVector(const std::vector<Notes::Event>& inNoteEvents)
{
    bool INCLUDE_PITCH_BENDS = false;
    // Compute size of single event vector
    size_t num_single_events = 0;

    for (const auto& note_event: inNoteEvents)
    {
        num_single_events += 2;

        if (INCLUDE_PITCH_BENDS)
        {
            // Don't include initial bend
            if (!note_event.bends.empty())
                num_single_events += (note_event.bends.size() - 1);
        }
    }

    std::vector<SingleEvent> out(num_single_events);

    size_t i = 0;

    for (const auto& note_event: inNoteEvents)
    {
        bool include_bends = INCLUDE_PITCH_BENDS && !note_event.bends.empty();
        float first_bend = include_bends ? float(note_event.bends[0]) / 3.0f : 0.0f;

        out[i++] = {note_event.startTime, note_event.pitch, NoteOn, first_bend};

        if (include_bends)
        {
            for (size_t j = 1; j < note_event.bends.size(); j++)
            {
                out[i++] = {note_event.startTime + double(j) * 0.011,
                            note_event.pitch,
                            PitchBend,
                            static_cast<float>(note_event.bends[j]) / 3.0f};
            }
        }

        out[i++] = {note_event.endTime, note_event.pitch, NoteOff, 0.0f};
        jassert(i <= num_single_events);
    }

    return out;
}

void Synth::setNewEventVectorToUse(std::vector<SingleEvent>& inEvents)
{
    const ScopedLock sl(mProcessor->getCallbackLock());

    std::swap(inEvents, mEvents);
    _updateCurrentEventIndex();
    _sanitizeVoices();
}

void Synth::_sanitizeVoices()
{
    // For each active voice, check if there's a noteOff event still there (before any noteOn with that midi note).
    // If not, shut down the voice.
    for (auto& voice: mVoices)
    {
        if (voice.isActive())
        {
            auto midi_note = voice.getCurrentNote();

            if (!_isNextOnOffEventNoteOff(midi_note))
            {
                voice.noteOff();
            }
        }
    }
}

void Synth::_updateCurrentEventIndex()
{
    mCurrentEventIndex =
        std::lower_bound(mEvents.begin(),
                         mEvents.end(),
                         mCurrentTime,
                         [](const SingleEvent& a, double b) { return (a.time < b); })
        - mEvents.begin();
}

bool Synth::_isNextOnOffEventNoteOff(int inMidiNote)
{
    auto iter =
        std::find_if(mEvents.begin() + static_cast<long>(mCurrentEventIndex),
                     mEvents.end(),
                     [inMidiNote](const SingleEvent& a) {
                         return (a.midiNote == inMidiNote) && (a.eventType != PitchBend);
                     });

    if (iter == mEvents.end())
    {
        return false;
    }

    return (*iter).eventType == NoteOff;
}
