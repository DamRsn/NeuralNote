//
// Created by Damien Ronssin on 29.08.26.
//

#include "NnGlobalSettings.h"

#include <JuceHeader.h>

#include "NNFileUtils.h"

namespace NnGlobalSettings
{

namespace
{

    const char* MODEL_SIZE_KEY = "modelSize";
    const char* EDITOR_SCALE_KEY = "editorScale";
    const char* TOOLTIPS_VISIBLE_KEY = "tooltipsVisible";
    const char* COMPUTE_DEVICE_KEY = "computeDevice";
    const char* COMPUTE_DEVICE_ORDINAL_KEY = "computeDeviceOrdinal";
    const char* CRASH_REPORTS_ENABLED_KEY = "crashReportsEnabled";
    const char* LOG_LEVEL_KEY = "logLevel";

    /**
     * Deleted by JUCE's own shutdown, while the message manager still exists: the file is a Timer,
     * and a timer must not outlive it.
     */
    struct Store : juce::DeletedAtShutdown {
        // Declared first so that it outlives the file, which takes a pointer to it and uses it while
        // saving from its own destructor.
        juce::InterProcessLock lock {"NeuralNoteGlobalSettings"};
        juce::PropertiesFile file;

        Store()
            : file(NNFileUtils::getGlobalSettingsFile(), _makeOptions(&lock))
        {
        }

        ~Store() override { clearSingletonInstance(); }

        static juce::PropertiesFile::Options _makeOptions(juce::InterProcessLock* inLock)
        {
            juce::PropertiesFile::Options options;

            // Negative leaves saving to us: a change is a set of keys, and _saveAllSettings
            // writes them in one go rather than one file write per setValue.
            options.millisecondsBeforeSaving = -1;
            options.storageFormat = juce::PropertiesFile::storeAsXML;

            // Several instances, in several hosts, can write at once.
            options.processLock = inLock;
            return options;
        }

        JUCE_DECLARE_SINGLETON_INLINE(Store, false)
    };

    juce::PropertiesFile& properties()
    {
        return Store::getInstance()->file;
    }

    /** Each getter means "what is stored, or the default in force", so writing them all back
        fills in the keys nobody has touched. */
    void _saveAllSettings()
    {
        auto& props = properties();

        props.setValue(MODEL_SIZE_KEY, modelSizeToString(getModelSize()));
        props.setValue(EDITOR_SCALE_KEY, getEditorScale());
        props.setValue(TOOLTIPS_VISIBLE_KEY, getTooltipsVisible());

        const ComputeDeviceChoice device = getComputeDevice();
        props.setValue(COMPUTE_DEVICE_KEY, juce::String(device.name));
        props.setValue(COMPUTE_DEVICE_ORDINAL_KEY, device.ordinal);
        props.setValue(CRASH_REPORTS_ENABLED_KEY, getCrashReportsEnabled());
        props.setValue(LOG_LEVEL_KEY, NnLogger::levelToString(getLogLevel()));

        props.save();
    }

} // namespace

void reload()
{
    properties().reload();
}

ModelSize getModelSize()
{
    const juce::String name = properties().getValue(MODEL_SIZE_KEY);
    return modelSizeFromString(name.toRawUTF8(), DEFAULT_MODEL_SIZE);
}

void setModelSize(ModelSize inModelSize)
{
    NnLogger::write(NnLogger::Level::Info,
                    "NnGlobalSettings: model size: " + juce::String(modelSizeToString(inModelSize)));
    properties().setValue(MODEL_SIZE_KEY, modelSizeToString(inModelSize));
    _saveAllSettings();
}

double getEditorScale()
{
    return properties().getDoubleValue(EDITOR_SCALE_KEY, 1.0);
}

void setEditorScale(double inScale)
{
    properties().setValue(EDITOR_SCALE_KEY, inScale);
    _saveAllSettings();
}

bool getTooltipsVisible()
{
    return properties().getBoolValue(TOOLTIPS_VISIBLE_KEY, true);
}

void setTooltipsVisible(bool inVisible)
{
    properties().setValue(TOOLTIPS_VISIBLE_KEY, inVisible);
    _saveAllSettings();
}

ComputeDeviceChoice getComputeDevice()
{
    return {.name = properties().getValue(COMPUTE_DEVICE_KEY).toStdString(),
            .ordinal = std::max(0, properties().getIntValue(COMPUTE_DEVICE_ORDINAL_KEY, 0))};
}

void setComputeDevice(const ComputeDeviceChoice& inChoice)
{
    NnLogger::write(NnLogger::Level::Info,
                    "NnGlobalSettings: compute device: "
                        + (inChoice.isAuto() ? juce::String("auto")
                                             : juce::String(inChoice.name) + " #" + juce::String(inChoice.ordinal)));
    properties().setValue(COMPUTE_DEVICE_KEY, juce::String(inChoice.name));
    properties().setValue(COMPUTE_DEVICE_ORDINAL_KEY, inChoice.ordinal);
    _saveAllSettings();
}

bool getCrashReportsEnabled()
{
    return properties().getBoolValue(CRASH_REPORTS_ENABLED_KEY, false);
}

void setCrashReportsEnabled(bool inEnabled)
{
    properties().setValue(CRASH_REPORTS_ENABLED_KEY, inEnabled);
    _saveAllSettings();
}

NnLogger::Level getLogLevel()
{
    return NnLogger::levelFromString(properties().getValue(LOG_LEVEL_KEY), NnLogger::Level::Info);
}

void setLogLevel(NnLogger::Level inLevel)
{
    properties().setValue(LOG_LEVEL_KEY, NnLogger::levelToString(inLevel));
    _saveAllSettings();
}

} // namespace NnGlobalSettings
