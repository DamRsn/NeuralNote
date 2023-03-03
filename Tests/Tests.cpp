#include <catch2/catch_test_macros.hpp>
#include <juce_core/juce_core.h>

template <typename T>
bool checkMin(T first, T second)
{
    return juce::jmin(first, second) == std::min(first, second);
}

template <typename T>
bool checkMax(T first, T second)
{
    return juce::jmax(first, second) == std::max(first, second);
}

TEST_CASE("Test that juce::jmin works")
{
    REQUIRE(checkMin(5, 7));
    REQUIRE(checkMin(12, 3));
    REQUIRE(checkMin(5.31, 5.42));
}

TEST_CASE("Test that juce::jmax works")
{
    REQUIRE(checkMax(5, 7));
    REQUIRE(checkMax(12, 3));
    REQUIRE(checkMax(5.31, 5.42));
}