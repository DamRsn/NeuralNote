//
// Created by Damien Ronssin on 12.10.26.
//

#include "NnLogger.h"

#include <utility>

#include "muscriptor/log.hpp"

#include "NNFileUtils.h"
#include "NnGlobalSettings.h"

namespace
{

constexpr juce::int64 MAX_INITIAL_FILE_SIZE_BYTES = 1024 * 1024;

// Covers everything below and the file writes.
juce::CriticalSection& getLock()
{
    static juce::CriticalSection lock;
    return lock;
}

NnLogger* gInstance = nullptr;
NnLogger::Level gLevel = NnLogger::Level::Info;

const char* levelTag(NnLogger::Level inLevel)
{
    switch (inLevel) {
        case NnLogger::Level::Debug:
            return "DEBUG";
        case NnLogger::Level::Info:
            return "INFO ";
        case NnLogger::Level::Warning:
            return "WARN ";
        case NnLogger::Level::Error:
            return "ERROR";
    }

    return "?    ";
}

juce::String threadTag()
{
    if (juce::MessageManager::existsAndIsCurrentThread()) {
        return "message";
    }

    if (const juce::Thread* thread = juce::Thread::getCurrentThread()) {
        return thread->getThreadName();
    }

    return juce::String::toHexString(reinterpret_cast<juce::pointer_sized_int>(juce::Thread::getCurrentThreadId()));
}

juce::String formatLine(NnLogger::Level inLevel, const juce::String& inMessage)
{
    const juce::Time now = juce::Time::getCurrentTime();

    return now.formatted("%Y-%m-%d %H:%M:%S.") + juce::String(now.getMilliseconds()).paddedLeft('0', 3) + " ["
           + levelTag(inLevel) + "] [" + threadTag() + "] " + inMessage;
}

NnLogger::Level fromMslLevel(msl::LogLevel inLevel)
{
    switch (inLevel) {
        case msl::LogLevel::Error:
            return NnLogger::Level::Error;
        case msl::LogLevel::Warn:
            return NnLogger::Level::Warning;
        case msl::LogLevel::Debug:
            return NnLogger::Level::Debug;
        case msl::LogLevel::Off:
        case msl::LogLevel::Info:
            break;
    }

    return NnLogger::Level::Info;
}

void onMslLog(msl::LogLevel inLevel, std::string_view inText)
{
    const juce::String text =
        juce::String::fromUTF8(inText.data(), static_cast<int>(inText.size())).trimCharactersAtEnd("\r\n");

    if (text.isNotEmpty()) {
        NnLogger::write(fromMslLevel(inLevel), "msl: " + text);
    }
}

} // namespace

NnLogger::NnLogger()
    : mFile(NNFileUtils::getLogsDirectory().getChildFile("NeuralNote.log"))
{
    juce::FileLogger::trimFileSize(mFile, MAX_INITIAL_FILE_SIZE_BYTES);
    mFile.create();

    const Level level = NnGlobalSettings::getLogLevel();

    {
        const juce::ScopedLock sl(getLock());
        jassert(gInstance == nullptr);
        gInstance = this;
        gLevel = level;
    }

    juce::Logger::setCurrentLogger(this);

    _writeSessionHeader();

    // Registered once, at the most verbose level: write() applies the current level, and msl's
    // callback must not be replaced while a model may be loading.
    msl::setLogCallback(onMslLog, msl::LogLevel::Debug);
}

NnLogger::~NnLogger()
{
    msl::setLogCallback({});

    if (juce::Logger::getCurrentLogger() == this) {
        juce::Logger::setCurrentLogger(nullptr);
    }

    const juce::ScopedLock sl(getLock());
    _append(Level::Info, "NnLogger: session ended");
    gInstance = nullptr;
}

void NnLogger::write(Level inLevel, const juce::String& inMessage)
{
    const juce::ScopedLock sl(getLock());

    if (gInstance == nullptr || inLevel < gLevel) {
        return;
    }

    gInstance->_append(inLevel, inMessage);
}

void NnLogger::setLevel(Level inLevel)
{
    const juce::ScopedLock sl(getLock());
    const Level previous = std::exchange(gLevel, inLevel);

    // Written whatever the level, so the log records being made quieter.
    if (gInstance != nullptr && previous != inLevel) {
        gInstance->_append(Level::Warning,
                           juce::String("NnLogger: log level: ") + levelToString(previous)
                               + juce::String(juce::CharPointer_UTF8(" \xe2\x86\x92 ")) + levelToString(inLevel));
    }
}

NnLogger::Level NnLogger::getLevel()
{
    const juce::ScopedLock sl(getLock());
    return gLevel;
}

const char* NnLogger::levelToString(Level inLevel)
{
    switch (inLevel) {
        case Level::Debug:
            return "debug";
        case Level::Info:
            return "info";
        case Level::Warning:
            return "warning";
        case Level::Error:
            return "error";
    }

    return "info";
}

NnLogger::Level NnLogger::levelFromString(const juce::String& inName, Level inDefault)
{
    for (const Level level: {Level::Debug, Level::Info, Level::Warning, Level::Error}) {
        if (inName == levelToString(level)) {
            return level;
        }
    }

    return inDefault;
}

void NnLogger::logMessage(const juce::String& inMessage)
{
    write(Level::Info, inMessage);
}

void NnLogger::_writeSessionHeader()
{
#if JUCE_DEBUG
    const char* build = "Debug";
#else
    const char* build = "Release";
#endif

#if JUCE_ARM
    const char* arch = "arm64";
#else
    const char* arch = "x86_64";
#endif

    const juce::PluginHostType host;
    juce::String settings = juce::String("log level ") + levelToString(getLevel());

#if JUCE_WINDOWS
    settings << ", crash reports " << (NnGlobalSettings::getCrashReportsEnabled() ? "on" : "off");
#endif

    const juce::StringArray lines {
        "NeuralNote " + juce::String(ProjectInfo::versionString) + " " + build + " " + arch,
        "Plugin format: "
            + juce::String(juce::AudioProcessor::getWrapperTypeDescription(juce::PluginHostType::getPluginLoadedAs())),
        "Host: " + juce::String(host.getHostDescription()) + " (" + juce::PluginHostType::getHostPath() + ")",
        "OS: " + juce::SystemStats::getOperatingSystemName(),
        "CPU: " + juce::SystemStats::getCpuModel() + ", " + juce::String(juce::SystemStats::getNumPhysicalCpus())
            + " cores, " + juce::String(juce::SystemStats::getNumCpus()) + " threads",
        "RAM: " + juce::String(juce::SystemStats::getMemorySizeInMegabytes()) + " MB",
        "JUCE: " + juce::SystemStats::getJUCEVersion(),
        "Settings: " + settings,
    };

    // Written whatever the level, like the end line: they delimit and identify the session.
    const juce::ScopedLock sl(getLock());

    for (const juce::String& line: lines) {
        _append(Level::Info, "NnLogger: " + line);
    }
}

void NnLogger::_append(Level inLevel, const juce::String& inMessage)
{
    const juce::String line = formatLine(inLevel, inMessage);
    DBG(line);

    juce::FileOutputStream out(mFile, 256);
    out << line << juce::newLine;
}
