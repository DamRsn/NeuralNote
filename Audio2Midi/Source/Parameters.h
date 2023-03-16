//#pragma once
//
//#include <JuceHeader.h>
//
//struct Parameters
//{
//    void add(juce::AudioProcessor& processor) const { processor.addParameter(mute); }
//
//    //Raw pointers. They will be owned by either the processor or the APVTS (if you use it)
//    juce::AudioParameterBool* mute =
//        new juce::AudioParameterBool({"Mute", 1}, "Mute", true);
//};