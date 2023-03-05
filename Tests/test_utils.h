//
// Created by Damien Ronssin on 05.03.23.
//

#ifndef AUDIO2MIDIPLUGIN_TEST_UTILS_H
#define AUDIO2MIDIPLUGIN_TEST_UTILS_H

#include <vector>

namespace test_utils
{
template <typename T>
std::vector<T> loadCSVDataFile(std::ifstream& stream)
{
    std::vector<T> vec;

    std::string line;
    if (stream.is_open())
    {
        while (std::getline(stream, line))
            vec.push_back(static_cast<T>(std::stod(line)));

        stream.close();
    }

    return vec;
}
} // namespace test_utils

#endif //AUDIO2MIDIPLUGIN_TEST_UTILS_H
