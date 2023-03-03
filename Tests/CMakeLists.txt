project(UnitTestRunner VERSION 0.1)

set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)
include(CTest)

#Declaring that we want 'catch2', a testing framework.
#Instructions on how to find it would appear under CMake/findcatch2.cmake
find_package(catch2 REQUIRED)

juce_add_console_app(UnitTestRunner PRODUCT_NAME "Unit Test Runner")

target_sources(UnitTestRunner PRIVATE Tests.cpp)

target_compile_definitions(UnitTestRunner PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0)

target_link_libraries(UnitTestRunner PRIVATE
        Catch2WithMain
        juce_recommended_config_flags
        juce_recommended_lto_flags
        juce_recommended_warning_flags
        juce_core)

catch_discover_tests(UnitTestRunner)