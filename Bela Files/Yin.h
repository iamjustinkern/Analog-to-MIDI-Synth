#pragma once
#include <vector>

class Yin {
public:
    Yin();
    void setup(float sampleRate);
    float process(const float* buffer, int bufferSize);
    float getProbability();

private:
    float sampleRate;
    float probability;
    float threshold;
    std::vector<float> yinBuffer;
};