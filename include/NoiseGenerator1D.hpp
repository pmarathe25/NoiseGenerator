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

        template <int width, int scaleX, typename InternalNoiseType>
        constexpr void fillLine(int internalX, int fillStartX, const InternalNoiseType& internalNoiseMap,
            TileMapF<width>& generatedNoiseMap, const TileMapF<scaleX>& attenuationsX) {
            // Only fill the part of the length that is valid.
            const int maxValidX = std::min(width - fillStartX, scaleX);
            // Cache noise values
            float left = internalNoiseMap(internalX);
            float right = internalNoiseMap(internalX + 1);
            // Loop over one interpolation kernel tile.
            for (int i = 0; i < maxValidX; ++i) {
                float attenuationX = attenuationsX(i);
                // Interpolate based on the 2 surrounding internal noise points.
                generatedNoiseMap(fillStartX + i) = interpolate1D(left, right, attenuationX);
            }
        }

    } /* Anonymous namespace */

    template <int width, int scaleX, typename Distribution = decltype(DefaultDistribution)>
    constexpr TileMapF<width> generate(Distribution&& distribution = DefaultDistribution) {
        // Get attenuation information
        const auto& attenuationsX{AttenuationsCache<scaleX>};
        // Generate a new internal noise map.
        constexpr int internalWidth = ceilDivide(width, scaleX) + 1;
        const auto& internalNoiseMap{generateInternalNoiseMap<internalWidth>(distribution)};
        // Interpolated noise
        TileMapF<width> generatedNoiseMap;
        // 1D noise map
        int fillStartX = 0;
        for (int i = 0; i < internalWidth - 1; ++i) {
            // 1D noise unit
            fillLine(i, fillStartX, internalNoiseMap, generatedNoiseMap, attenuationsX);
            fillStartX += scaleX;
        }
        // Return noise map.
        return generatedNoiseMap;
    }

    template <int width, int scaleX, int numOctaves = 8, typename Distribution>
    constexpr TileMapF<width> generateOctavesImpl(Distribution&& distribution, float multiplier, float decayFactor) {
        // This multiplier should equal the last one if this is the final octave.
        if constexpr (numOctaves == 1) return (multiplier / decayFactor) * generate<width, scaleX>(std::forward<Distribution&&>(distribution));
        else return multiplier * generate<width, scaleX>(std::forward<Distribution&&>(distribution)) + generateOctavesImpl<width, ceilDivide(scaleX, 2),
            numOctaves - 1>(std::forward<Distribution&&>(distribution), multiplier * decayFactor, decayFactor);
    }

    // Convenience overloads
    template <int width, int scaleX, int numOctaves = 8, typename Distribution = decltype(DefaultDistribution)>
    constexpr TileMapF<width> generateOctaves(Distribution&& distribution = DefaultDistribution, float multiplier = 0.5f, float decayFactor = 0.5f) {
        return generateOctavesImpl<width, scaleX, numOctaves>(std::forward<Distribution&&>(distribution), multiplier, decayFactor);
    }
} /* StealthNoiseGenerator */

#endif /* end of include guard: NOISE_GENERATOR_1D_H */
