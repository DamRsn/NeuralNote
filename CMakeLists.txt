cmake_minimum_required(VERSION 3.16)
project(Audio2MidiPlugin)

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

# Add JUCE
add_subdirectory(JUCE)
add_subdirectory(Audio2Midi)

#By default, we are building the provided plugin/app examples:
option(BUILD_JUCE_PROTOTYPE_EXAMPLES "Build JUCE prototype examples" ON)

#optionally, we're also adding the unit tests:
option(BUILD_UNIT_TESTS "Build JUCE prototype examples" ON)

if (BUILD_UNIT_TESTS)
    enable_testing()
    add_subdirectory(Tests)
endif()
