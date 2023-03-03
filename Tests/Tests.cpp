#include <JuceHeader.h>

template <typename T>
bool checkMax(T first, T second)
{
    return juce::jmax(first, second) == std::max(first, second);
}

int main()
{
    if (checkMax(1, 2))
    {
        std::cout << "SUCCESS" << std::endl;
        return 0;
    }
    else
    {
        std::cout << "ERROR" << std::endl;
        return 1;
    }
}