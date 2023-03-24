//
// Created by Damien Ronssin on 19.03.23.
//

#ifndef NN_RHYTHMUTILS_H
#define NN_RHYTHMUTILS_H

#include <JuceHeader.h>

namespace RhythmUtils
{
static const juce::StringArray TimeDivisionsStr {"1/1",
                                                 "1/2",
                                                 "1/3",
                                                 "1/4",
                                                 "1/6",
                                                 "1/8",
                                                 "1/12",
                                                 "1/16",
                                                 "1/24",
                                                 "1/32",
                                                 "1/48",
                                                 "1/64"};

enum TimeDivisions
{
    _1_1 = 0,
    _1_2,
    _1_3,
    _1_4,
    _1_6,
    _1_8,
    _1_12,
    _1_16,
    _1_24,
    _1_32,
    _1_48,
    _1_64,
    TotalNumTimeDivision
};

const static std::array<double, TotalNumTimeDivision> TimeDivisionsDouble = {1.0 / 1.0,
                                                                             1.0 / 2.0,
                                                                             1.0 / 3.0,
                                                                             1.0 / 4.0,
                                                                             1.0 / 6.0,
                                                                             1.0 / 8.0,
                                                                             1.0 / 12.0,
                                                                             1.0 / 16.0,
                                                                             1.0 / 24.0,
                                                                             1.0 / 32.0,
                                                                             1.0 / 48.0,
                                                                             1.0 / 64.0};

} // namespace RhythmUtils

#endif //NN_RHYTHMUTILS_H
