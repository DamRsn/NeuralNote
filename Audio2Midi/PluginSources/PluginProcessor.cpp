#include "PluginProcessor.h"
#include "PluginEditor.h"

Audio2MidiAudioProcessor::Audio2MidiAudioProcessor()
{
    parameters.add(*this);
    mAudioToConvert.reserve(mMaxNumSamplesToConvert);
}

void Audio2MidiAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    mDownSampler.prepareToPlay(sampleRate, samplesPerBlock);
}

void Audio2MidiAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                            juce::MidiBuffer& midiMessages)

{
    juce::ignoreUnused(midiMessages);

    if (parameters.enable->get())
        buffer.applyGain(parameters.gain->get());
    else
        buffer.clear();

    if(parameters.record->get()) {
//        std::copy()
    }
}

juce::AudioProcessorEditor* Audio2MidiAudioProcessor::createEditor()
{
    return new Audio2MidiEditor(*this);
}

void Audio2MidiAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
}

void Audio2MidiAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Audio2MidiAudioProcessor();
}
