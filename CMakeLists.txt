cmake_minimum_required(VERSION 3.24)
project(NeuralNotePlugin VERSION 2.0.0)

# C++ settings
set(CMAKE_CXX_STANDARD 23)
enable_language(CXX)

# stb_vorbis is C and does not compile as C++.
enable_language(C)

#Compile commands, useful for some IDEs like VS-Code
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

#Minimum MacOS target, set globally. Matches muscriptor.cpp's floor.
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum OS X deployment version" FORCE)

option(NEURALNOTE_MUSCRIPTOR_DEBUG "Build muscriptor.cpp in Debug even when the plugin is Debug" OFF)
option(LTO "Enable Link Time Optimization" ON)
option(NEURALNOTE_FETCH_SOUNDFONT "Download the soundfont at configure time if it is missing" ON)

#static linking in Windows
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_subdirectory(ThirdParty/JUCE)
add_subdirectory(ThirdParty/TinySoundFont)
add_subdirectory(ThirdParty/minimp3)

# The soundfont the synth plays is ~38 MB and is not committed; Tools/fetch_soundfont.py
# downloads and SHA-256-verifies it. Run here so a fresh clone builds in one step.
# Failing at configure time with the script named beats failing at link time with a
# missing BinaryData symbol.
set(NEURALNOTE_SOUNDFONT "${CMAKE_CURRENT_LIST_DIR}/NeuralNote/Assets/SoundFont/MuseScore_General.sf3")

if (NEURALNOTE_FETCH_SOUNDFONT AND NOT EXISTS "${NEURALNOTE_SOUNDFONT}")
  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  message(STATUS "Fetching the soundfont (~38 MB), one time only")
  execute_process(
          COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_LIST_DIR}/Tools/fetch_soundfont.py"
          RESULT_VARIABLE soundfont_result)
  if (NOT soundfont_result EQUAL 0)
    message(FATAL_ERROR "Could not fetch the soundfont. Run Tools/fetch_soundfont.py by hand.")
  endif ()
endif ()

if (NOT EXISTS "${NEURALNOTE_SOUNDFONT}")
  message(FATAL_ERROR
          "Missing ${NEURALNOTE_SOUNDFONT}.\n"
          "Run Tools/fetch_soundfont.py, or configure with -DNEURALNOTE_FETCH_SOUNDFONT=ON.")
endif ()

# muscriptor.cpp pins CMake >=3.24 and forces itself + ggml to build as
# static libraries (required inside a plugin bundle). BUILD_TESTING=OFF avoids
# pulling its Catch2/nlohmann_json test dependencies into this build.
set(BUILD_TESTING OFF)
add_subdirectory(ThirdParty/muscriptor.cpp/cpp muscriptor)

# muscriptor.cpp (and the ggml it fetches) can't follow a single-config
# generator's global CMAKE_BUILD_TYPE selectively, so force it to optimized
# code regardless of whether the plugin itself is built Debug or Release.
# NEURALNOTE_MUSCRIPTOR_DEBUG opts back into following the plugin's build type.
if (NOT NEURALNOTE_MUSCRIPTOR_DEBUG)
  foreach (muscriptor_target muscriptor_ggml ggml ggml-base ggml-cpu ggml-metal pffft)
    if (TARGET ${muscriptor_target})
      if (MSVC)
        target_compile_options(${muscriptor_target} PRIVATE /O2 /DNDEBUG)
      else ()
        target_compile_options(${muscriptor_target} PRIVATE -O3 -DNDEBUG)
      endif ()
    endif ()
  endforeach ()
endif ()

set(BaseTargetName NeuralNote)

# Set COPY_PLUGIN_AFTER_BUILD to TRUE on Mac and to FALSE on Windows due to permission issues
if (APPLE)
  set(COPY_PLUGIN_AFTER_BUILD TRUE)
else ()
  set(COPY_PLUGIN_AFTER_BUILD FALSE)
endif ()

juce_add_plugin("${BaseTargetName}"
        # VERSION ...                               # Set this if the plugin version is different to the project version
        ICON_BIG "${CMAKE_CURRENT_LIST_DIR}/NeuralNote/Assets/logo.png"
        ICON_SMALL "${CMAKE_CURRENT_LIST_DIR}/NeuralNote/Assets/logo.png"
        COMPANY_NAME "Dr. Audio"
        BUNDLE_ID "com.draudio.neuralnote"
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT TRUE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD ${COPY_PLUGIN_AFTER_BUILD}
        MICROPHONE_PERMISSION_ENABLED TRUE
        MICROPHONE_PERMISSION_TEXT "Need access to Microphone"
        HARDENED_RUNTIME_ENABLED TRUE
        PLUGIN_MANUFACTURER_CODE DrAu
        PLUGIN_CODE NRNT
        FORMATS AU VST3 Standalone
        # Only acted on for Linux, the one platform JUCE_USE_CURL is set for below.
        NEEDS_CURL TRUE
        PRODUCT_NAME "NeuralNote")

juce_generate_juce_header(${BaseTargetName})

# Source files
file(GLOB_RECURSE SOURCES_PLUGIN ${CMAKE_CURRENT_LIST_DIR}/NeuralNote/*.cpp ${CMAKE_CURRENT_LIST_DIR}/Lib/*.cpp)
file(GLOB_RECURSE HEADERS_PLUGIN ${CMAKE_CURRENT_LIST_DIR}/NeuralNote/*.h ${CMAKE_CURRENT_LIST_DIR}/Lib/*.h)

target_sources(${BaseTargetName} PRIVATE ${SOURCES_PLUGIN} ${HEADERS_PLUGIN})

file(GLOB_RECURSE lib_dirs LIST_DIRECTORIES true ${CMAKE_CURRENT_LIST_DIR}/Lib/*)

foreach (dir ${lib_dirs})
  IF (IS_DIRECTORY ${dir})
    target_include_directories(${BaseTargetName} PRIVATE ${dir})
  ELSE ()
    CONTINUE()
  ENDIF ()
endforeach ()

file(GLOB_RECURSE source_dirs LIST_DIRECTORIES true ${CMAKE_CURRENT_LIST_DIR}/NeuralNote/*)

foreach (dir ${source_dirs})
  IF (IS_DIRECTORY ${dir})
    target_include_directories(${BaseTargetName} PRIVATE ${dir})
  ELSE ()
    CONTINUE()
  ENDIF ()
endforeach ()

#Binary data
file(GLOB RESOURCES_FILES ${CMAKE_CURRENT_LIST_DIR}/NeuralNote/Assets/Fonts/*.ttf)

# The soundfont is its own binary-data target: it is ~38 MB against a few hundred KB
# of fonts and images, and keeping it separate means editing an icon does not
# regenerate (and recompile) the soundfont's source file.
juce_add_binary_data(bin_data SOURCES ${RESOURCES_FILES})
# Its own namespace, or its generated BinaryData.h would collide with bin_data's.
juce_add_binary_data(soundfont_data NAMESPACE SoundFontData HEADER_NAME SoundFontData.h
        SOURCES "${NEURALNOTE_SOUNDFONT}")

target_compile_definitions(${BaseTargetName}
        PUBLIC
        JUCE_WEB_BROWSER=0
        # Without curl, JUCE's Linux networking only speaks plain HTTP, and model downloads are HTTPS.
        JUCE_USE_CURL=$<IF:$<PLATFORM_ID:Linux>,1,0>
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(${BaseTargetName}
        PRIVATE
        juce::juce_audio_utils
        juce::juce_cryptography
        juce::juce_dsp
        muscriptor_ggml
        tinysoundfont
        minimp3
        bin_data
        soundfont_data
        PUBLIC
        juce_recommended_config_flags
        juce_recommended_warning_flags)

if (${LTO})
  target_link_libraries(${BaseTargetName} PUBLIC juce_recommended_lto_flags)
  message(STATUS "Link Time Optimization (LTO) enabled")
endif ()

if (BUILD_UNIT_TESTS)
  add_subdirectory(Tests)
endif ()

if (WIN32)
  target_compile_definitions(${BaseTargetName} PUBLIC JUCE_ASIO=1)
  target_include_directories(${BaseTargetName} PRIVATE ThirdParty/ASIO/common)
endif ()
