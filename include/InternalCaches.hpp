#ifndef STEALTH_INTERPOLATION_H
#define STEALTH_INTERPOLATION_H
#include "TileMap/TileMap.hpp"
#include <stealthutil>

namespace StealthNoiseGenerator {
    namespace {
        template <int scale>
        constexpr StealthTileMap::TileMapF<scale> generateAttenuations() noexcept {
            StealthTileMap::TileMapF<scale> attenuations;
            for (int i = 0; i < scale; ++i) {
                attenuations(i) = attenuationPolynomial((i + 0.5f) / scale);
            }
            return attenuations;
        }

        // Attenuations cache
        template <int scale>
        static const StealthTileMap::TileMapF<scale> AttenuationsCache{std::move(generateAttenuations<scale>())};

        template <int size>
        static StealthTileMap::TileMapF<size> InternalNoiseMapCache{};

        // Initialize with random values according to provided distribution
        template <int width, int length = 1, int height = 1, typename Distribution>
        constexpr const auto& generateInternalNoiseMap(Distribution& distribution) {
            // Internal noise map should be large enough to fit tiles of size (scale, scale).
            constexpr int size = width * length * height;
            for (int i = 0; i < size; ++i) {
                InternalNoiseMapCache<size>(i) = distribution(generator);
            }
            return InternalNoiseMapCache<size>;
        }
    } /* Anonymous namespace */
} /* StealthWorldGenerator */

#endif
