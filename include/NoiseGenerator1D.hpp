#ifndef NOISE_GENERATOR_1D_H
#define NOISE_GENERATOR_1D_H
#include "InternalCaches.hpp"
#include "TileMap/TileMap.hpp"
#include <stealthutil>
#include <random>

namespace StealthNoiseGenerator {
    namespace {
        constexpr float interpolate1D(float left, float right, float attenuation) noexcept {
            // Interpolate between two points
            float nx = left * (1.0f - attenuation) + right * attenuation;
            return nx;
        }

        template <int width, typename overwrite, int scaleX, typename InternalNoiseType, typename GeneratedNoiseType>
        constexpr void fillLine(int internalX, int fillStartX, const InternalNoiseType& internalNoiseMap,
            GeneratedNoiseType& generatedNoiseMap, const TileMapF<scaleX>& attenuationsX, float multiplier = 1.0f) {
            // Only fill the part of the length that is valid.
            const int maxValidX = std::min(width - fillStartX, scaleX);
            // Cache noise values
            float left = internalNoiseMap(internalX);
            float right = internalNoiseMap(internalX + 1);
            // Loop over one interpolation kernel tile.
            for (int i = 0; i < maxValidX; ++i) {
                float attenuationX = attenuationsX(i);
                // Interpolate based on the 2 surrounding internal noise points.
                if constexpr (overwrite::value) {
                    generatedNoiseMap(fillStartX + i) = interpolate1D(left, right, attenuationX);
                } else {
                    generatedNoiseMap(fillStartX + i) += interpolate1D(left, right, attenuationX) * multiplier;
                }
            }
        }
    } /* Anonymous namespace */

    template <int width, int scaleX, typename overwrite = std::true_type,
        typename Distribution = decltype(DefaultDistribution), typename GeneratedNoiseType>
    constexpr GeneratedNoiseType& generate(GeneratedNoiseType& generatedNoiseMap, Distribution&& distribution
        = std::forward<Distribution&&>(DefaultDistribution), long seed = stealth::getCurrentTime(),
        float multiplier = 1.0f) {
        if constexpr (scaleX == 1) {
            if constexpr (overwrite::value) generatedNoiseMap.randomize(std::forward<Distribution&&>(distribution), seed);
            else generatedNoiseMap += GeneratedNoiseType::Random(std::forward<Distribution&&>(distribution),
                    seed, std::default_random_engine{}) * multiplier;
            return generatedNoiseMap;
        }
        // Get attenuation information
        const auto& attenuationsX{AttenuationsCache<scaleX>};
        // Generate a new internal noise map.
        constexpr int internalWidth = stealth::ceilDivide(width, scaleX) + 1;
        const auto& internalNoiseMap{generateInternalNoiseMap<internalWidth>(seed,
            std::forward<Distribution&&>(distribution))};
        // 1D noise map
        int fillStartX = 0;
        for (int i = 0; i < internalWidth - 1; ++i) {
            // 1D noise unit
            fillLine<width, overwrite>(i, fillStartX, internalNoiseMap, generatedNoiseMap, attenuationsX, multiplier);
            fillStartX += scaleX;
        }
        // Return noise map.
        return generatedNoiseMap;
    }

    // Return a normalization factor
    template <int width, int scaleX, int numOctaves = 6, typename Distribution, typename GeneratedNoiseType>
    constexpr float generateOctaves1D_impl(GeneratedNoiseType& generatedNoiseMap, long seed,
        Distribution&& distribution, float decayFactor, float accumulator = 1.0f) {
        // First generate this layer...
        generate<width, scaleX, std::false_type>(generatedNoiseMap, std::forward<Distribution&&>(distribution),
            seed, accumulator);
        // ...then generate the next octaves.
        if constexpr (numOctaves > 1) {
            return accumulator + generateOctaves1D_impl<width, stealth::ceilDivide(scaleX, 2), numOctaves - 1>
                (generatedNoiseMap, seed, std::forward<Distribution&&>(distribution), decayFactor, accumulator * decayFactor);
        } else {
            return accumulator;
        }
    }

    // Convenience overloads
    template <int width, int scaleX, int numOctaves = 6, typename overwrite = std::true_type,
        typename Distribution = decltype(DefaultDistribution), typename GeneratedNoiseType>
    constexpr GeneratedNoiseType& generateOctaves(GeneratedNoiseType& generatedNoiseMap, Distribution&& distribution
        = std::forward<Distribution&&>(DefaultDistribution), long seed = stealth::getCurrentTime(), float decayFactor = 0.5f) {
        // Zero if we need to overwrite
        if constexpr (overwrite::value) generatedNoiseMap = 0.0f;
        // Generate and normalize!
        generatedNoiseMap /= generateOctaves1D_impl<width, scaleX, numOctaves>(generatedNoiseMap, seed,
            std::forward<Distribution&&>(distribution), decayFactor);
        return generatedNoiseMap;
    }
} /* StealthNoiseGenerator */

#endif /* end of include guard: NOISE_GENERATOR_1D_H */
