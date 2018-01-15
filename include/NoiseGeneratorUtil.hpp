#ifndef NOISE_GENERATOR_UTIL_H
#define NOISE_GENERATOR_UTIL_H
#define CURRENT_TIME std::chrono::system_clock::now().time_since_epoch().count()
#include "TileMap/TileMap.hpp"
#include <random>
#include <chrono>

namespace StealthNoiseGenerator {
    namespace {
        using StealthTileMap::TileMapF;
        // Maintain a cache of interpolation kernels of different sizes
        static inline std::default_random_engine generator{CURRENT_TIME};

        constexpr float findDecayFactor(float multiplier) {
            // Treat as an infinite geometric sum where multiplier = alpha and decayFactor = r
            return (1.0f - multiplier <= 0.0f) ? 0.0001f : 1.0f - multiplier;
        }

        // Initialize with random values according to provided distribution
        template <int internalWidth, int internalLength = 1, int internalHeight = 1, typename Distribution>
        constexpr auto generateInternalNoiseMap(Distribution& distribution) {
            // Internal noise map should be large enough to fit tiles of size (scale, scale).
            StealthTileMap::TileMapF<internalWidth, internalLength, internalHeight> internalNoiseMap;
            for (int i = 0; i < internalNoiseMap.size(); ++i) {
                internalNoiseMap(i) = distribution(generator);
            }
            return internalNoiseMap;
        }
    }
} /* StealthNoiseGenerator */

#undef CURRENT_TIME
#endif /* end of include guard: NOISE_GENERATOR_UTIL_H */
