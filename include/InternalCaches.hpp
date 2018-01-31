#ifndef STEALTH_INTERPOLATION_H
#define STEALTH_INTERPOLATION_H
#include "TileMap/TileMap.hpp"
#include <stealthutil>
#include <random>

namespace StealthNoiseGenerator {
    namespace {
        constexpr float attenuationPolynomial(float distance) noexcept {
            return (6 * pow(distance, 5) - 15 * pow(distance, 4) + 10 * pow(distance, 3));
        }

        std::mt19937 DefaultGenerator;
        std::uniform_real_distribution DefaultDistribution{0.0f, 1.0f};

        using StealthTileMap::TileMapF;

        // Attenuations cache
        template <int scale>
        constexpr TileMapF<scale> generateAttenuations() noexcept {
            TileMapF<scale> attenuations;
            for (int i = 0; i < scale; ++i) {
                attenuations(i) = attenuationPolynomial(i / (float) scale);
            }
            return attenuations;
        }

        template <int scale>
        const TileMapF<scale> AttenuationsCache{std::move(generateAttenuations<scale>())};

        // Prevent extra allocations
        template <int size>
        TileMapF<size> InternalNoiseMapCache{};

        // Initialize with random values according to provided distribution
        template <int width, int length = 1, int height = 1, typename Distribution,
            typename Generator = decltype(DefaultGenerator)>
        constexpr const auto& generateInternalNoiseMap(long desiredSeed, Distribution&& distribution,
            Generator&& generator = std::forward<Generator&&>(DefaultGenerator)) {
            // Internal noise map should be large enough to fit tiles of size (scale, scale).
            generator.seed(desiredSeed);
            constexpr int size = width * length * height;
            for (int i = 0; i < size; ++i) {
                InternalNoiseMapCache<size>(i) = distribution(generator);
            }
            return InternalNoiseMapCache<size>;
        }
    } /* Anonymous namespace */
} /* StealthWorldGenerator */

#endif
