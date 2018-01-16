#ifndef NOISE_GENERATOR_UTIL_H
#define NOISE_GENERATOR_UTIL_H
#include <chrono>

namespace StealthNoiseGenerator {
    inline auto getCurrentTime() {
        return std::chrono::system_clock::now().time_since_epoch().count();
    }

    constexpr float findDecayFactor(float multiplier) {
        // Treat as an infinite geometric sum where multiplier = alpha and decayFactor = r
        return (1.0f - multiplier <= 0.0f) ? 0.0001f : 1.0f - multiplier;
    }
} /* StealthNoiseGenerator */

#endif /* end of include guard: NOISE_GENERATOR_UTIL_H */
