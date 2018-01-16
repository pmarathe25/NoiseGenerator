#ifndef NOISE_GENERATOR_H
#define NOISE_GENERATOR_H
#include "NoiseGenerator1D.hpp"
#include "NoiseGenerator2D.hpp"
#include "NoiseGenerator3D.hpp"

namespace StealthNoiseGenerator {
    constexpr float findDecayFactor(float multiplier) {
        // Treat as an infinite geometric sum where multiplier = alpha and decayFactor = r
        return (1.0f - multiplier <= 0.0f) ? 0.0001f : 1.0f - multiplier;
    }
} /* StealthNoiseGenerator */

#endif
