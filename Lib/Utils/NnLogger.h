//
// Created by Damien Ronssin on 12.10.26.
//

#ifndef NnLogger_h
#define NnLogger_h

#include <JuceHeader.h>

/**
 * Writes NeuralNote's log to <NeuralNote>/logs/NeuralNote.log, one line per message:
 * "2026-10-12 14:03:22.123 [INFO ] [message] Component: text".
 *
 * One per process, shared through juce::SharedResourcePointer. While it exists it is JUCE's current
 * logger and the sink for msl's (and so ggml's) log messages.
 *
 * Never log from the audio thread: write() takes a lock and opens the file.
 */
class NnLogger : private juce::Logger
{
public:
    enum class Level { Debug, Info, Warning, Error };

    /** Message thread only: reads the level from NnGlobalSettings. */
    NnLogger();

    ~NnLogger() override;

    /** Thread-safe. Drops the message when no NnLogger exists or when it is below the current level. */
    static void write(Level inLevel, const juce::String& inMessage);

    /** Takes effect immediately for every thread. Does not save the level: the caller does. */
    static void setLevel(Level inLevel);

    static Level getLevel();

    static const char* levelToString(Level inLevel);

    /** inName as levelToString writes it, or inDefault. */
    static Level levelFromString(const juce::String& inName, Level inDefault);

private:
    /** What JUCE and juce::Logger::writeToLog send, at Info. */
    void logMessage(const juce::String& inMessage) override;

    void _writeSessionHeader();

    /** Bypasses the level. Caller holds the lock. */
    void _append(Level inLevel, const juce::String& inMessage);

    const juce::File mFile;

    JUCE_DECLARE_NON_COPYABLE(NnLogger)
};

#endif // NnLogger_h
