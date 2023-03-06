#include <JuceHeader.h>
#include "Features.h"
#include "BasicPitchCNN.h"
#include <vector>
#include "test_utils.h"
#include "features_test.h"
#include "cnn_test.h"
#include "perf_test.h"
#include "notes_test.h"

int main()
{
    int result = 0;

    std::cout << std::endl << "FEATURE TEST" << std::endl;
    result |= !feature_test();

    std::cout << std::endl << "CNN TEST" << std::endl;
    result |= !cnn_test();

    std::cout << std::endl << "PERF TEST" << std::endl;
    result |= !perf_test();

    std::cout << std::endl << "NOTES TEST" << std::endl;
    result |= !notes_test();

    return result;
}