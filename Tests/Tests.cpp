#include <JuceHeader.h>
#include "Features.h"
#include <vector>

int main()
{
    std::vector<float> x;

    Features feature_calculator;
    size_t num_input_samples = 100000;
    x.resize(num_input_samples);

    size_t num_out_frames;

    const float* stacked_cqt_data =
        feature_calculator.computeFeatures(x.data(), num_input_samples, num_out_frames);

    std::cout << num_out_frames << std::endl;



}