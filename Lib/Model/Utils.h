//
// Created by Tibor Vass on 07.03.23.
//

#ifndef Utils_h
#define Utils_h

#include <assert.h>
#include <cstdlib>

static int safe_divide(int a, int b)
{
    auto res = std::div(a, b);
    assert(res.rem == 0);
    return res.quot;
}

#endif // Utils_h
