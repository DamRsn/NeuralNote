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

} // namespace NnGlobalSettings
