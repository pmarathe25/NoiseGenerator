#ifndef NOISE_GENERATOR_2D_H
#define NOISE_GENERATOR_2D_H
#include "NoiseGenerator1D.hpp"
#include "InternalCaches.hpp"
#include "TileMap/TileMap.hpp"
#include "NoiseGeneratorUtil.hpp"
#include <stealthutil>
#include <random>

namespace StealthNoiseGenerator {
    namespace {
        constexpr float interpolate2D(float topLeft, float topRight, float bottomLeft,
            float bottomRight, float attenuationX, float attenuationY) noexcept {
            // Interpolate horizontally
            float nx0 = interpolate1D(topLeft, topRight, attenuationX);
            float nx1 = interpolate1D(bottomLeft, bottomRight, attenuationX);
            // Interpolate vertically
            float nxy = interpolate1D(nx0, nx1, attenuationY);
            return nxy;
        }

        template <int width, int length, typename overwrite, int scaleX, int scaleY,
            typename InternalNoiseType, typename GeneratedNoiseType>
        constexpr void fillSquare(int internalX, int internalY, int fillStartX, int fillStartY,
            const InternalNoiseType& internalNoiseMap, GeneratedNoiseType& generatedNoiseMap,
            const TileMapF<scaleX>& attenuationsX, const TileMapF<scaleY>& attenuationsY,
            float multiplier = 1.0f) {
            // Only fill the part of the tile that is valid.
            const int maxValidX = std::min(width - fillStartX, scaleX);
            const int maxValidY = std::min(length - fillStartY, scaleY);
            // Cache noise indices
            constexpr int internalWidth = ceilDivide(width, scaleX) + 1;
            const int topLeftIndex = internalX + internalY * internalWidth;
            const int bottomLeftIndex = topLeftIndex + internalWidth;
            // Cache noise values
            float topLeft = internalNoiseMap[topLeftIndex];
            float topRight = internalNoiseMap[topLeftIndex + 1];
            float bottomLeft = internalNoiseMap[bottomLeftIndex];
            float bottomRight = internalNoiseMap[bottomLeftIndex + 1];
            // Loop over one interpolation kernel tile.
            int index = fillStartX + fillStartY * generatedNoiseMap.width();
            for (int j = 0; j < maxValidY; ++j) {
                float attenuationY = attenuationsY[j];
                for (int i = 0; i < maxValidX; ++i) {
                    float attenuationX = attenuationsX[i];
                    // Interpolate based on the 4 surrounding internal noise points.
                    if constexpr (overwrite::value) {
                        generatedNoiseMap[index++] = interpolate2D(topLeft, topRight, bottomLeft,
                            bottomRight, attenuationX, attenuationY);
                    } else {
                        generatedNoiseMap[index++] += interpolate2D(topLeft, topRight, bottomLeft,
                            bottomRight, attenuationX, attenuationY) * multiplier;
                    }
                }
                // Wrap around to the first element of the next row.
                index += generatedNoiseMap.width() - maxValidX;
            }
        }
    } /* Anonymous namespace */

    template <int width, int length, int scaleX, int scaleY, typename overwrite
        = std::true_type, typename Seed = decltype(getCurrentTime()), typename Distribution
        = decltype(DefaultDistribution), typename GeneratedNoiseType>
    constexpr GeneratedNoiseType& generate(GeneratedNoiseType& generatedNoiseMap, Seed&& seed = getCurrentTime(),
        Distribution&& distribution = std::forward<Distribution&&>(DefaultDistribution), float multiplier = 1.0f) {
        // Generate 1D noise if there's only 1 dimension.
        if constexpr (length == 1) {
            return generate<width, scaleX, overwrite>(generatedNoiseMap, std::forward<Seed&&>(seed),
                std::forward<Distribution&&>(distribution), multiplier);
        } else if constexpr (width == 1) {
            return generate<length, scaleY, overwrite>(generatedNoiseMap, std::forward<Seed&&>(seed),
                std::forward<Distribution&&>(distribution), multiplier);
        }
        // Get attenuation information
        const auto& attenuationsX{AttenuationsCache<scaleX>};
        const auto& attenuationsY{AttenuationsCache<scaleY>};
        // Generate a new internal noise map.
        constexpr int internalWidth = ceilDivide(width, scaleX) + 1;
        constexpr int internalLength = ceilDivide(length, scaleY) + 1;
        const auto& internalNoiseMap{generateInternalNoiseMap<internalWidth, internalLength>
            (std::forward<Seed&&>(seed), std::forward<Distribution&&>(distribution))};
        // 2D noise map
        int fillStartX = 0, fillStartY = 0;
        for (int j = 0; j < internalLength - 1; ++j) {
            fillStartX = 0;
            for (int i = 0; i < internalWidth - 1; ++i) {
                // 2D noise unit
                fillSquare<width, length, overwrite>(i, j, fillStartX, fillStartY,
                    internalNoiseMap, generatedNoiseMap, attenuationsX, attenuationsY, multiplier);
                fillStartX += scaleX;
            }
            fillStartY += scaleY;
        }
        // Return noise map.
        return generatedNoiseMap;
    }

    template <int width, int length, int scaleX, int scaleY, int numOctaves = 8, typename Seed,
        typename Distribution, typename GeneratedNoiseType>
    constexpr void generateOctaves2DImpl(GeneratedNoiseType& generatedNoiseMap, Seed&& seed,
        Distribution&& distribution, float multiplier, float decayFactor) {
        if constexpr (numOctaves == 1) {
            // This multiplier should equal the last one if this is the final octave.
            generate<width, length, scaleX, scaleY, std::false_type>(generatedNoiseMap, std::forward<Seed&&>(seed),
                std::forward<Distribution&&>(distribution), (multiplier / decayFactor));
        } else {
            // First generate this layer...
            generate<width, length, scaleX, scaleY, std::false_type>(generatedNoiseMap, std::forward<Seed&&>(seed),
                std::forward<Distribution&&>(distribution), multiplier);
            // ...then generate the next octaves.
            generateOctaves2DImpl<width, length, ceilDivide(scaleX, 2), ceilDivide(scaleY, 2), numOctaves - 1>
                (generatedNoiseMap, std::forward<Seed&&>(seed), std::forward<Distribution&&>(distribution),
                multiplier * decayFactor, decayFactor);
        }
    }

    // Convenience overloads
    template <int width, int length, int scaleX, int scaleY, int numOctaves = 8, typename overwrite
        = std::true_type, typename Seed = decltype(getCurrentTime()), typename Distribution
        = decltype(DefaultDistribution), typename GeneratedNoiseType>
    constexpr GeneratedNoiseType& generateOctaves(GeneratedNoiseType& generatedNoiseMap, Seed&& seed
        = getCurrentTime(), Distribution&& distribution = std::forward<Distribution&&>(DefaultDistribution),
        float multiplier = 0.5f, float decayFactor = 0.5f) {
        if constexpr (overwrite::value) {
            generatedNoiseMap = 0.0f;
        }
        generateOctaves2DImpl<width, length, scaleX, scaleY, numOctaves>(generatedNoiseMap,
            std::forward<Seed&&>(seed), std::forward<Distribution&&>(distribution), multiplier, decayFactor);
        return generatedNoiseMap;
    }
} /* StealthNoiseGenerator */

#endif /* end of include guard: NOISE_GENERATOR_2D_H */
