project(GuiAppTemplate VERSION 0.0.1)

set (TargetName ${PROJECT_NAME})

juce_add_gui_app(${TargetName} PRODUCT_NAME "Gui App Example")
juce_generate_juce_header(${TargetName})

set(source_files
        Window/Main.cpp
        Window/MainWindow.cpp
        Window/MainWindow.h

        Source/CommonHeader.h
        Source/HelloWorldLabel.h
        Source/MainComponent.h
        Source/MainComponent.cpp)

target_sources(${TargetName} PRIVATE ${source_files})

source_group(TREE ${CMAKE_CURRENT_LIST_DIR} PREFIX "SOURCE FILES" FILES ${source_files})

target_compile_definitions(${TargetName} PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:${TargetName},JUCE_PROJECT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:${TargetName},JUCE_VERSION>")

target_link_libraries(${TargetName} PRIVATE
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
        juce::juce_gui_extra)
