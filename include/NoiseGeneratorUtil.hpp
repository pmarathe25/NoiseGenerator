#ifndef NOISE_GENERATOR_UTIL_H
#define NOISE_GENERATOR_UTIL_H
#include <chrono>

namespace StealthNoiseGenerator {
    inline auto getCurrentTime() {
        return std::chrono::system_clock::now().time_since_epoch().count();
    }
} /* StealthNoiseGenerator */

#endif /* end of include guard: NOISE_GENERATOR_UTIL_H */
