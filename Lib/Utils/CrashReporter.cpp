//
// Created by Damien Ronssin on 09.10.26.
//

#include "CrashReporter.h"

#if JUCE_WINDOWS

#include <atomic>
#include <csignal>
#include <cstdio>
#include <string>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dbghelp.h>

#include "NNFileUtils.h"
#include "NnGlobalSettings.h"
#include "NnLogger.h"

#pragma comment(lib, "dbghelp.lib")

namespace
{

// Set before the handlers are installed: after a crash, the heap may be what broke.
std::wstring gDirectory;

LPTOP_LEVEL_EXCEPTION_FILTER gPreviousFilter = nullptr;
decltype(SIG_DFL) gPreviousAbortHandler = SIG_DFL;

enum DumpState
{
    Idle,
    Writing,
    Written
};

std::atomic<int> gDumpState {Idle};

void writeMinidump(EXCEPTION_POINTERS* inException)
{
    // One dump per process: an uncaught C++ exception reaches the filter, then abort().
    int expected = Idle;

    if (!gDumpState.compare_exchange_strong(expected, Writing)) {
        // Another thread crashing meanwhile gives the first one time to finish.
        if (expected == Writing) {
            Sleep(10000);
        }

        return;
    }

    SYSTEMTIME time;
    GetLocalTime(&time);

    wchar_t path[MAX_PATH * 2];
    std::swprintf(path,
                  std::size(path),
                  L"%lsNeuralNote_crash_%04u-%02u-%02u_%02u-%02u-%02u_%lu.dmp",
                  gDirectory.c_str(),
                  time.wYear,
                  time.wMonth,
                  time.wDay,
                  time.wHour,
                  time.wMinute,
                  time.wSecond,
                  GetCurrentProcessId());

    const HANDLE file = CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (file != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION exception_info {GetCurrentThreadId(), inException, FALSE};
        const auto type = static_cast<MINIDUMP_TYPE>(MiniDumpWithIndirectlyReferencedMemory | MiniDumpWithThreadInfo
                                                     | MiniDumpWithUnloadedModules);

        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file, type, &exception_info, nullptr, nullptr);
        CloseHandle(file);
    }

    gDumpState = Written;
}

LONG WINAPI onException(EXCEPTION_POINTERS* inException)
{
    writeMinidump(inException);

    return gPreviousFilter != nullptr ? gPreviousFilter(inException) : EXCEPTION_CONTINUE_SEARCH;
}

// abort() raises SIGABRT, then ends the process without reaching the exception filter. The static
// CRT is this binary's own, so only aborts in NeuralNote's code come here.
void onAbort(int)
{
    // MiniDumpWriteDump cannot reliably walk the calling thread's stack without a captured context.
    CONTEXT context {};
    RtlCaptureContext(&context);

    EXCEPTION_RECORD record {};
    record.ExceptionCode = 0x40000015; // STATUS_FATAL_APP_EXIT

    EXCEPTION_POINTERS pointers {&record, &context};
    writeMinidump(&pointers);
}

} // namespace

CrashReporter::CrashReporter()
{
    if (NnGlobalSettings::getCrashReportsEnabled()) {
        _install();
    }
}

CrashReporter::~CrashReporter()
{
    if (mEnabled) {
        _uninstall();
    }
}

void CrashReporter::setEnabled(bool inEnabled)
{
    if (inEnabled && !mEnabled) {
        _install();
    } else if (!inEnabled && mEnabled) {
        _uninstall();
    }

    if (inEnabled == mEnabled) {
        NnLogger::write(NnLogger::Level::Info, juce::String("CrashReporter: ") + (mEnabled ? "enabled" : "disabled"));
    } else {
        NnLogger::write(NnLogger::Level::Error, "CrashReporter: could not create the crash reports folder");
    }

    NnGlobalSettings::setCrashReportsEnabled(mEnabled);
}

bool CrashReporter::isEnabled() const
{
    return mEnabled;
}

void CrashReporter::_install()
{
    const juce::File directory = NNFileUtils::getCrashReportsDirectory();

    if (!NNFileUtils::ensureDirectoryExists(directory)) {
        return;
    }

    gDirectory = directory.getFullPathName().toWideCharPointer();
    gDirectory += L'\\';

    gPreviousFilter = SetUnhandledExceptionFilter(&onException);
    gPreviousAbortHandler = std::signal(SIGABRT, &onAbort);
    mEnabled = true;
}

void CrashReporter::_uninstall()
{
    // A filter installed after ours is left in place.
    const LPTOP_LEVEL_EXCEPTION_FILTER current = SetUnhandledExceptionFilter(gPreviousFilter);

    if (current != &onException) {
        SetUnhandledExceptionFilter(current);
    }

    std::signal(SIGABRT, gPreviousAbortHandler);
    mEnabled = false;
}

#endif
