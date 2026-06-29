#include "Yin.h"

Yin::Yin() : sampleRate(44100.0f), probability(0.0f), threshold(0.15f) {}

void Yin::setup(float sr) {
    sampleRate = sr;
    // Pre-allocate memory to prevent dropouts on Bela's high-priority audio thread
    yinBuffer.resize(2048, 0.0f); 
}

float Yin::process(const float* buffer, int bufferSize) {
    int halfBufferSize = bufferSize / 2;
    
    // Zero out our calculation buffer
    for(int i = 0; i < halfBufferSize; i++) {
        yinBuffer[i] = 0.0f;
    }

    // 1. Difference function
    for (int tau = 1; tau < halfBufferSize; tau++) {
        for (int i = 0; i < halfBufferSize; i++) {
            float delta = buffer[i] - buffer[i + tau];
            yinBuffer[tau] += delta * delta;
        }
    }

    // 2. Cumulative mean normalized difference
    yinBuffer[0] = 1.0f;
    float runningSum = 0.0f;
    for (int tau = 1; tau < halfBufferSize; tau++) {
        runningSum += yinBuffer[tau];
        yinBuffer[tau] *= tau / runningSum;
    }

    // 3. Absolute thresholding (Find the first dip below our threshold)
    int tauEstimate = -1;
    for (int tau = 2; tau < halfBufferSize; tau++) {
        if (yinBuffer[tau] < threshold) {
            while (tau + 1 < halfBufferSize && yinBuffer[tau + 1] < yinBuffer[tau]) {
                tau++;
            }
            tauEstimate = tau;
            break;
        }
    }

    // If no pitch is confident enough, return 0
    if (tauEstimate == -1) {
        probability = 0.0f;
        return 0.0f; 
    }

    // 4. Parabolic interpolation (Refine the estimate for better accuracy)
    float betterTau = tauEstimate;
    if (tauEstimate > 0 && tauEstimate < halfBufferSize - 1) {
        float s0 = yinBuffer[tauEstimate - 1];
        float s1 = yinBuffer[tauEstimate];
        float s2 = yinBuffer[tauEstimate + 1];
        betterTau += (s0 - s2) / (2.0f * (s0 - 2.0f * s1 + s2));
    }

    // Calculate confidence based on the depth of the dip
    probability = 1.0f - yinBuffer[tauEstimate];
    
    // Convert the period (tau) back into Frequency (Hz)
    return sampleRate / betterTau;
}

float Yin::getProbability() {
    return probability;
}