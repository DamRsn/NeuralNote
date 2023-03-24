//
// Created by Damien Ronssin on 05.03.23.
//

#ifndef NN_TEST_UTILS_H
#define NN_TEST_UTILS_H

#include <assert.h>
#include <cstdlib>
#include <stdexcept>
#include <vector>

#include "Utils.h"

namespace test_utils
{
template <typename T>
static std::vector<T> loadCSVDataFile(std::ifstream& stream)
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

template <typename T>
static std::vector<std::vector<T>>
    convert_1d_to_2d(std::vector<T> flattened_vec, int rows, int cols)
{
    if (rows < 0)
    {
        if (cols < 0)
        {
            throw std::runtime_error(
                "convert_1d_to_2d: either rows or cols have to be specified (i.e. positive)");
        }
        rows = safe_divide(flattened_vec.size(), cols);
    }
    else if (cols < 0)
    {
        cols = safe_divide(flattened_vec.size(), rows);
    }
    else if (flattened_vec.size() != rows * cols)
    {
        throw std::runtime_error(std::string("convert_1d_to_2d: invalid size of vector")
                                 + std::to_string(flattened_vec.size())
                                 + std::string(". Expected ")
                                 + std::to_string(rows * cols));
    }

    std::vector<std::vector<T>> result(rows, std::vector<T>(cols));
    for (int i = 0; i < rows; i++)
    {
        std::copy(flattened_vec.begin() + i * cols,
                  flattened_vec.begin() + (i + 1) * cols,
                  result[i].begin());
    }
    return result;
}
} // namespace test_utils

#endif //NN_TEST_UTILS_H
