project(UnitTests VERSION 1.0)

juce_add_console_app(${PROJECT_NAME} PRODUCT_NAME "Unit Tests")

juce_generate_juce_header(${PROJECT_NAME})

file(GLOB_RECURSE SOURCES_TESTS ${CMAKE_CURRENT_LIST_DIR}/../Lib/*.cpp)
file(GLOB_RECURSE HEADERS_TESTS ${CMAKE_CURRENT_LIST_DIR}/../Lib/*.h)

target_sources(${PROJECT_NAME} PRIVATE Tests.cpp ${SOURCES_TESTS} ${HEADERS_TESTS})

file(GLOB_RECURSE lib_sources LIST_DIRECTORIES true ${CMAKE_CURRENT_LIST_DIR}/../Lib/*)

foreach (dir ${lib_sources})
    IF (IS_DIRECTORY ${dir})
        target_include_directories(${PROJECT_NAME} PRIVATE ${dir})
    ELSE ()
        CONTINUE()
    ENDIF ()
endforeach ()

target_compile_definitions(${PROJECT_NAME} PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0)

target_link_libraries(${PROJECT_NAME} PRIVATE
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
        juce_recommended_lto_flags
        juce_recommended_warning_flags)



