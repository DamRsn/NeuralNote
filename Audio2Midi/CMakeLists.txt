project(Audio2Midi VERSION 0.0.1)

set (BaseTargetName Audio2Midi)

juce_add_plugin("${BaseTargetName}"
        # VERSION ...                               # Set this if the plugin version is different to the project version
        # ICON_BIG ...                              # ICON_* arguments specify a path to an image file to use as an icon for the Standalone
        # ICON_SMALL ...
        COMPANY_NAME "DamienRonssin"
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD TRUE
        PLUGIN_MANUFACTURER_CODE Damr
        PLUGIN_CODE AdMd
        FORMATS AU VST3 Standalone
        PRODUCT_NAME "Audio To Midi")

juce_generate_juce_header(${PROJECT_NAME})

target_sources(${BaseTargetName} PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/ProcessorBase.cpp
        Model/BasicPitchCNN.cpp)

target_compile_definitions(${BaseTargetName}
        PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(${BaseTargetName} PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        RTNeural
        juce_recommended_config_flags
        juce_recommended_lto_flags
        juce_recommended_warning_flags)
