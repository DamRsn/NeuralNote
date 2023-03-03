#include <juce_core/juce_core.h>

extern "C" void dllFunction();

void dllFunction()
{
    juce::Logger::writeToLog("Logging from the DLL!");
}
