#ifndef NOISE_GENERATOR_2D_H
#define NOISE_GENERATOR_2D_H
#include "NoiseGeneratorUtil.hpp"
#include "AttenuationsCache.hpp"
#include "TileMap/TileMap.hpp"
#include <stealthutil>
#include <random>

namespace StealthNoiseGenerator {
    constexpr float interpolate2D(float topLeft, float topRight, float bottomLeft,
        float bottomRight, float attenuationX, float attenuationY) noexcept {
        // Interpolate horizontally
        float nx0 = interpolate1D(topLeft, topRight, attenuationX);
        float nx1 = interpolate1D(bottomLeft, bottomRight, attenuationX);
        // Interpolate vertically
        float nxy = interpolate1D(nx0, nx1, attenuationY);
        return nxy;
    }

    template <int width, int length, int scaleX, int scaleY, int internalWidth, int internalLength>
    constexpr void fillSquare(int internalX, int internalY, int fillStartX, int fillStartY,
        const StealthTileMap::TileMapF<internalWidth, internalLength>& internalNoiseMap,
        StealthTileMap::TileMapF<width, length>& generatedNoiseMap, const TileMapF<scaleX>& attenuationsX,
        const TileMapF<scaleY>& attenuationsY) {
        // Only fill the part of the tile that is valid.
        const int maxValidX = std::min(width - fillStartX, scaleX);
        const int maxValidY = std::min(length - fillStartY, scaleY);
        // Cache noise indices
        const int topLeftIndex = internalX + internalY * internalNoiseMap.width();
        const int bottomLeftIndex = topLeftIndex + internalNoiseMap.width();
        // Cache noise values
        float topLeft = internalNoiseMap(topLeftIndex);
        float topRight = internalNoiseMap(topLeftIndex + 1);
        float bottomLeft = internalNoiseMap(bottomLeftIndex);
        float bottomRight = internalNoiseMap(bottomLeftIndex + 1);
        // Loop over one interpolation kernel tile.
        int index = fillStartX + fillStartY * generatedNoiseMap.width();
        for (int j = 0; j < maxValidY; ++j) {
            for (int i = 0; i < maxValidX; ++i) {
                // Interpolate based on the 4 surrounding internal noise points.
                generatedNoiseMap(index++) = interpolate2D(topLeft, topRight, bottomLeft,
                    bottomRight, attenuationsX(i), attenuationsY(j));
            }
            // Wrap around to the first element of the next row.
            index += generatedNoiseMap.width() - maxValidX;
        }
    }

    template <int width, int length, int scaleX, int scaleY, typename Distribution = std::uniform_real_distribution<float>>
    constexpr StealthTileMap::TileMapF<width, length> generate(Distribution&& distribution
    = std::uniform_real_distribution{0.0f, 1.0f}) {
        // Generate 1D noise if there's only 1 dimension.
        if constexpr (length == 1) {
            return generate<width, scaleX>(std::forward<Distribution&&>(distribution));
        } else if constexpr (width == 1) {
            return generate<length, scaleY>(std::forward<Distribution&&>(distribution));
        }
        // Get attenuation information
        const auto attenuationsX = AttenuationsCache<scaleX>;
        const auto attenuationsY = AttenuationsCache<scaleY>;
        // Generate a new internal noise map.
        constexpr int internalWidth = ceilDivide(width, scaleX) + 1;
        constexpr int internalLength = ceilDivide(length, scaleY) + 1;
        const auto internalNoiseMap = generateInternalNoiseMap<internalWidth, internalLength>(distribution);
        // Interpolated noise
        StealthTileMap::TileMapF<width, length> generatedNoiseMap;
        // 2D noise map
        int fillStartX = 0, fillStartY = 0;
        for (int j = 0; j < internalLength - 1; ++j) {
            fillStartX = 0;
            for (int i = 0; i < internalWidth - 1; ++i) {
                // 2D noise unit
                fillSquare(i, j, fillStartX, fillStartY, internalNoiseMap, generatedNoiseMap, attenuationsX, attenuationsY);
                fillStartX += scaleX;
            }
            fillStartY += scaleY;
        }
        // Return noise map.
        return generatedNoiseMap;
    }

    template <int width, int length, int scaleX, int scaleY, int numOctaves = 8, typename Distribution = std::uniform_real_distribution<float>>
    constexpr StealthTileMap::TileMapF<width, length> generateOctaves(Distribution&& distribution = std::uniform_real_distribution{0.0f, 1.0f},
        float multiplier = 0.5f, float decayFactor = 0.5f) {
        // This multiplier should equal the last one if this is the final octave.
        if constexpr (numOctaves == 1) return (multiplier / decayFactor) * generate<width, length, scaleX, scaleY>(std::forward<Distribution&&>(distribution));
        else return multiplier * generate<width, length, scaleX, scaleY>(std::forward<Distribution&&>(distribution))
            + generateOctaves<width, length, ceilDivide(scaleX, 2), ceilDivide(scaleY, 2),
            numOctaves - 1>(std::forward<Distribution&&>(distribution), multiplier * decayFactor, decayFactor);
    }

    // Convenience overloads
    template <int width, int length, int scaleX, int scaleY, int numOctaves = 8, typename Distribution = std::uniform_real_distribution<float>>
    constexpr StealthTileMap::TileMapF<width, length> generateOctaves(Distribution&& distribution, float multiplier) {
        return generateOctaves<width, length, scaleX, scaleY, numOctaves>
            (std::forward<Distribution&&>(distribution), multiplier, findDecayFactor(multiplier));
    }

    template <int width, int length, int scaleX, int scaleY, int numOctaves = 8, typename Distribution = std::uniform_real_distribution<float>>
    constexpr StealthTileMap::TileMapF<width, length> generateOctaves(float multiplier) {
        return generateOctaves<width, length, scaleX, scaleY, numOctaves>
            (std::uniform_real_distribution{0.0f, 1.0f}, multiplier, findDecayFactor(multiplier));
    }

    template <int width, int length, int scaleX, int scaleY, int numOctaves = 8, typename Distribution = std::uniform_real_distribution<float>>
    constexpr StealthTileMap::TileMapF<width, length> generateOctaves(float multiplier, float decayFactor) {
        return generateOctaves<width, length, scaleX, scaleY, numOctaves>
            (std::uniform_real_distribution{0.0f, 1.0f}, multiplier, decayFactor);
    }
} /* StealthNoiseGenerator */

#endif /* end of include guard: NOISE_GENERATOR_2D_H */
