cmake_minimum_required(VERSION 3.16)
project(Audio2MidiPlugin VERSION 0.0.1)

# C++ settings
set(CMAKE_CXX_STANDARD 17)
enable_language(CXX)

#Compile commands, useful for some IDEs like VS-Code
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

#Minimum MacOS target, set globally
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.11" CACHE STRING "Minimum OS X deployment version" FORCE)

option(UniversalBinary "Build universal binary for mac" OFF)

if (UniversalBinary)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE INTERNAL "")
endif()

#static linking in Windows
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_subdirectory(JUCE)
add_subdirectory(RTNeural)

set(BaseTargetName Audio2Midi)

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

juce_generate_juce_header(${BaseTargetName})

# Source files
file(GLOB SOURCES_PLUGIN ${CMAKE_CURRENT_LIST_DIR}/Audio2Midi/*/*.cpp)
file(GLOB HEADERS_PLUGIN ${CMAKE_CURRENT_LIST_DIR}/Audio2Midi/*/*.h)

target_sources(${BaseTargetName} PRIVATE ${SOURCES_PLUGIN} ${HEADERS_PLUGIN})

file(GLOB RESOURCES_FILES ${CMAKE_CURRENT_LIST_DIR}/Audio2Midi/ModelData/*.json)
juce_add_binary_data(bin_data SOURCES ${RESOURCES_FILES})

target_include_directories("${BaseTargetName}" PRIVATE ${CMAKE_CURRENT_LIST_DIR}/ONNXRuntime/${ONNXRUNTIME_DIRNAME}/include)

target_compile_definitions(${BaseTargetName}
        PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

# Import ONNXRuntime dynamic library
# Directory containing onnxruntime
if (APPLE)
    set(ONNXRUNTIME_DIRNAME onnxruntime-osx-universal2-1.14.1)
elseif(WIN32)
    set(ONNXRUNTIME_DIRNAME onnxruntime-win-x64-1.14.1)
endif()

add_library(onnxruntime SHARED IMPORTED)

if (APPLE)
    set_property(TARGET onnxruntime PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/ONNXRuntime/${ONNXRUNTIME_DIRNAME}/lib/libonnxruntime.dylib)

elseif(WIN32)
    set_property(TARGET onnxruntime APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)

    set_target_properties(onnxruntime PROPERTIES
            IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
            IMPORTED_LOCATION_RELEASE "${CMAKE_CURRENT_LIST_DIR}/ThirdParty/${ONNXRUNTIME_DIRNAME}/lib/onnxruntime.dll"
            IMPORTED_IMPLIB_RELEASE "${CMAKE_CURRENT_LIST_DIR}/ThirdParty/${ONNXRUNTIME_DIRNAME}/lib/onnxruntime.lib"
            )
    set_target_properties(onnxruntime PROPERTIES
            MAP_IMPORTED_CONFIG_DEBUG Release
            MAP_IMPORTED_CONFIG_MINSIZEREL Release
            MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
            )
endif()


target_include_directories("${BaseTargetName}" PRIVATE ${CMAKE_CURRENT_LIST_DIR}/ONNXRuntime/${ONNXRUNTIME_DIRNAME}/include)

target_link_libraries(${BaseTargetName}
        PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        RTNeural
        onnxruntime
        bin_data
        PUBLIC
        juce_recommended_config_flags
        juce_recommended_lto_flags
        juce_recommended_warning_flags)

if (BUILD_UNIT_TESTS)
    add_subdirectory(Tests)
endif()
