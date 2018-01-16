#ifndef STEALTH_INTERPOLATION_H
#define STEALTH_INTERPOLATION_H
#include "TileMap/TileMap.hpp"
#include <stealthutil>
#include <random>

namespace StealthNoiseGenerator {
    static std::default_random_engine DefaultGenerator{};
    static std::uniform_real_distribution DefaultDistribution{0.0f, 1.0f};

    namespace {
        using StealthTileMap::TileMapF;

        // Attenuations cache
        template <int scale>
        constexpr TileMapF<scale> generateAttenuations() noexcept {
            TileMapF<scale> attenuations;
            for (int i = 0; i < scale; ++i) {
                attenuations(i) = attenuationPolynomial((i + 0.5f) / scale);
            }
            return attenuations;
        }

        template <int scale>
        static const TileMapF<scale> AttenuationsCache{std::move(generateAttenuations<scale>())};

        // Prevent extra allocations
        template <int size>
        static TileMapF<size> InternalNoiseMapCache{};

        // Initialize with random values according to provided distribution
        template <int width, int length = 1, int height = 1, typename Distribution,
            typename Seed, typename Generator = decltype(DefaultGenerator)>
        constexpr const auto& generateInternalNoiseMap(Seed&& desiredSeed,
            Distribution&& distribution, Generator&& generator = std::forward<Generator&&>(DefaultGenerator)) {
            // Internal noise map should be large enough to fit tiles of size (scale, scale).
            generator.seed(std::forward<Seed&&>(desiredSeed));
            constexpr int size = width * length * height;
            for (int i = 0; i < size; ++i) {
                InternalNoiseMapCache<size>[i] = distribution(generator);
            }
            return InternalNoiseMapCache<size>;
        }
    } /* Anonymous namespace */
} /* StealthWorldGenerator */

#endif
