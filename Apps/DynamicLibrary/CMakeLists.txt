project(AppWithDLL VERSION 0.1)

#first, we define the dll:
add_library(DLL SHARED Source/DLL.cpp)
target_compile_definitions(DLL PRIVATE JUCE_STANDALONE_APPLICATION=1)
_juce_initialise_target(DLL)

#hiding all symbols, except the C interface:
set_property(TARGET DLL PROPERTY C_VISIBILITY_PRESET hidden)
set_property(TARGET DLL PROPERTY VISIBILITY_INLINES_HIDDEN ON)

target_link_libraries(DLL PRIVATE
        juce_recommended_config_flags
        juce_recommended_lto_flags
        juce_recommended_warning_flags
        juce_core)

#Now, the app:
set (TargetName ${PROJECT_NAME})
juce_add_console_app(${TargetName} PRODUCT_NAME ${TargetName})

target_sources(${TargetName} PRIVATE Source/Main.cpp)

target_compile_definitions(${TargetName} PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0)

target_link_libraries(${TargetName} PRIVATE
        juce_recommended_config_flags
        juce_recommended_lto_flags
        juce_recommended_warning_flags
        juce_core)

#even though we don't link the DLL directly with the app,
# we depend on the DLL so they both will build:
add_dependencies(${TargetName} DLL)

#We'll make sure the DLL is copied to where the binary is, for convenience
get_target_property(path ${TargetName} RUNTIME_OUTPUT_DIRECTORY)

add_custom_command(TARGET DLL
        POST_BUILD
        COMMAND ${CMAKE_COMMAND}
        -E copy $<TARGET_FILE:DLL> ${path})
