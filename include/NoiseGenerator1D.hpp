#ifndef NOISE_GENERATOR_1D_H
#define NOISE_GENERATOR_1D_H
#include "InternalCaches.hpp"
#include "TileMap/TileMap.hpp"
#include "NoiseGeneratorUtil.hpp"
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
            float left = internalNoiseMap[internalX];
            float right = internalNoiseMap[internalX + 1];
            // Loop over one interpolation kernel tile.
            for (int i = 0; i < maxValidX; ++i) {
                float attenuationX = attenuationsX[i];
                // Interpolate based on the 2 surrounding internal noise points.
                if constexpr (overwrite::value) {
                    generatedNoiseMap[fillStartX + i] = interpolate1D(left, right, attenuationX);
                } else {
                    generatedNoiseMap[fillStartX + i] += interpolate1D(left, right, attenuationX) * multiplier;
                }
            }
        }
    } /* Anonymous namespace */

    template <int width, int scaleX, typename overwrite = std::true_type, typename Seed = decltype(getCurrentTime()),
        typename Distribution = decltype(DefaultDistribution), typename GeneratedNoiseType>
    constexpr GeneratedNoiseType& generate(GeneratedNoiseType& generatedNoiseMap,
        Distribution&& distribution = std::forward<Distribution&&>(DefaultDistribution),
        Seed&& seed = getCurrentTime(), float multiplier = 1.0f) {
        // Get attenuation information
        const auto& attenuationsX{AttenuationsCache<scaleX>};
        // Generate a new internal noise map.
        constexpr int internalWidth = ceilDivide(width, scaleX) + 1;
        const auto& internalNoiseMap{generateInternalNoiseMap<internalWidth>(std::forward<Seed&&>(seed),
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

    template <int width, int scaleX, int numOctaves = 8, typename Distribution, typename Seed, typename GeneratedNoiseType>
    constexpr void generateOctaves1DImpl(GeneratedNoiseType& generatedNoiseMap, Seed&& seed, Distribution&& distribution, float multiplier, float decayFactor) {
        if constexpr (numOctaves == 1) {
            // This multiplier should equal the last one if this is the final octave.
            generate<width, scaleX, std::false_type>(generatedNoiseMap,
                std::forward<Distribution&&>(distribution), std::forward<Seed&&>(seed), (multiplier / decayFactor));
        } else {
            // First generate this layer...
            generate<width, scaleX, std::false_type>(generatedNoiseMap,
                std::forward<Distribution&&>(distribution), std::forward<Seed&&>(seed), multiplier);
            // ...then generate the next octaves.
            generateOctaves1DImpl<width, ceilDivide(scaleX, 2), numOctaves - 1>(generatedNoiseMap, std::forward<Seed&&>(seed),
                std::forward<Distribution&&>(distribution), multiplier * decayFactor, decayFactor);
        }
    }

    // Convenience overloads
    template <int width, int scaleX, int numOctaves = 8, typename overwrite = std::true_type,
        typename Seed = decltype(getCurrentTime()), typename Distribution = decltype(DefaultDistribution),
        typename GeneratedNoiseType>
    constexpr GeneratedNoiseType& generateOctaves(GeneratedNoiseType& generatedNoiseMap,
        Distribution&& distribution = std::forward<Distribution&&>(DefaultDistribution),
        Seed&& seed = getCurrentTime(), float multiplier = 0.5f, float decayFactor = 0.5f) {
        if constexpr (overwrite::value) {
            generatedNoiseMap = 0.0f;
        }
        generateOctaves1DImpl<width, scaleX, numOctaves>(generatedNoiseMap, std::forward<Seed&&>(seed),
            std::forward<Distribution&&>(distribution), multiplier, decayFactor);
        return generatedNoiseMap;
    }
} /* StealthNoiseGenerator */

#endif /* end of include guard: NOISE_GENERATOR_1D_H */
