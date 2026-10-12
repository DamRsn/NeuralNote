//
// Created by Damien Ronssin on 09.10.26.
//

#ifndef CrashReporter_h
#define CrashReporter_h

#include <JuceHeader.h>

#if JUCE_WINDOWS

/**
 * Opt-in: writes a minidump to NNFileUtils::getCrashReportsDirectory() when the process crashes,
 * then hands the crash on to the handler that was there before.
 *
 * One per process, shared through juce::SharedResourcePointer: the last instance puts the previous
 * handlers back. Message thread only.
 */
class CrashReporter
{
public:
    /** Applies NnGlobalSettings::getCrashReportsEnabled(). */
    CrashReporter();

    ~CrashReporter();

    /** Installs or removes the handlers, and saves the setting. Stays off if the directory cannot be created. */
    void setEnabled(bool inEnabled);

    bool isEnabled() const;

private:
    void _install();
    void _uninstall();

    bool mEnabled = false;

    JUCE_DECLARE_NON_COPYABLE(CrashReporter)
};

#endif

#endif // CrashReporter_h
