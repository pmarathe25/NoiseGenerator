#ifndef NOISE_GENERATOR_3D_H
#define NOISE_GENERATOR_3D_H
#include "NoiseGeneratorUtil.hpp"
#include "AttenuationsCache.hpp"
#include "TileMap/TileMap.hpp"
#include <stealthutil>
#include <random>

namespace StealthNoiseGenerator {
    namespace {
        constexpr float interpolate3D(float topLeft0, float topRight0, float bottomLeft0, float bottomRight0,
            float topLeft1, float topRight1, float bottomLeft1, float bottomRight1,
            float attenuationX, float attenuationY, float attenuationZ) noexcept {
                // Interpolate bottom layer
                float nz0 = interpolate2D(topLeft0, topRight0, bottomLeft0, bottomRight0, attenuationX, attenuationY);
                float nz1 = interpolate2D(topLeft1, topRight1, bottomLeft1, bottomRight1, attenuationX, attenuationY);
                // Interpolate between two layers
                float nxyz = interpolate1D(nz0, nz1, attenuationZ);
                return nxyz;
            }

        template <int width, int length, int height, int scaleX, int scaleY, int scaleZ, int internalWidth, int internalLength, int internalHeight>
        constexpr void fillCube(int internalX, int internalY, int internalZ, int fillStartX, int fillStartY, int fillStartZ,
            const StealthTileMap::TileMapF<internalWidth, internalLength, internalHeight>& internalNoiseMap,
            StealthTileMap::TileMapF<width, length, height>& generatedNoiseMap, const TileMapF<scaleX>& attenuationsX,
            const TileMapF<scaleY>& attenuationsY, const TileMapF<scaleZ>& attenuationsZ) {
            // Only fill the part of the tile that is valid.
            const int maxValidX = std::min(width - fillStartX, scaleX);
            const int maxValidY = std::min(length - fillStartY, scaleY);
            const int maxValidZ = std::min(height - fillStartZ, scaleZ);
            // Cache noise indices
            const int topLeft0Index = internalX + internalY * internalNoiseMap.width() + internalZ * internalNoiseMap.area();
            const int bottomLeft0Index = topLeft0Index + internalNoiseMap.width();
            const int topLeft1Index = topLeft0Index + internalNoiseMap.area();
            const int bottomLeft1Index = bottomLeft0Index + internalNoiseMap.area();
            // Cache noise values
            float topLeft0 = internalNoiseMap(topLeft0Index);
            float topRight0 = internalNoiseMap(topLeft0Index + 1);
            float bottomLeft0 = internalNoiseMap(bottomLeft0Index);
            float bottomRight0 = internalNoiseMap(bottomLeft0Index + 1);
            float topLeft1 = internalNoiseMap(topLeft1Index);
            float topRight1 = internalNoiseMap(topLeft1Index + 1);
            float bottomLeft1 = internalNoiseMap(bottomLeft1Index);
            float bottomRight1 = internalNoiseMap(bottomLeft1Index + 1);
            // Loop over one interpolation kernel tile.
            int index = fillStartX + fillStartY * generatedNoiseMap.width() + fillStartZ * generatedNoiseMap.area();
            for (int k = 0; k < maxValidZ; ++k) {
                for (int j = 0; j < maxValidY; ++j) {
                    for (int i = 0; i < maxValidX; ++i) {
                        // Interpolate based on the 4 surrounding internal noise points.
                        generatedNoiseMap(index++) = interpolate3D(topLeft0, topRight0, bottomLeft0, bottomRight0,
                            topLeft1, topRight1, bottomLeft1, bottomRight1,
                            attenuationsX(i), attenuationsY(j), attenuationsZ(k));
                    }
                    // Wrap around to the first element of the next row.
                    index += generatedNoiseMap.width() - maxValidX;
                }
                // Wrap around to the first element of the next tile
                index += generatedNoiseMap.area() - maxValidY * generatedNoiseMap.width();
            }
        }
    }

    template <int width, int length, int height, int scaleX, int scaleY, int scaleZ, typename Distribution
        = std::uniform_real_distribution<float>>
    constexpr StealthTileMap::TileMapF<width, length, height> generate(Distribution&& distribution
        = std::uniform_real_distribution{0.0f, 1.0f}) {
        // Generate 2D noise if there are only 2 dimensions.
        if constexpr (height == 1) {
            return generate<width, length, scaleX, scaleY>(std::forward<Distribution&&>(distribution));
        } else if constexpr (length == 1) {
            return generate<width, height, scaleX, scaleZ>(std::forward<Distribution&&>(distribution));
        } else if constexpr (width == 1) {
            return generate<length, height, scaleY, scaleZ>(std::forward<Distribution&&>(distribution));
        }
        const auto attenuationsX = AttenuationsCache<scaleX>;
        const auto attenuationsY = AttenuationsCache<scaleY>;
        const auto attenuationsZ = AttenuationsCache<scaleZ>;
        // Generate a new internal noise map.
        constexpr int internalWidth = ceilDivide(width, scaleX) + 1;
        constexpr int internalLength = ceilDivide(length, scaleY) + 1;
        constexpr int internalHeight = ceilDivide(height, scaleZ) + 1;
        const auto internalNoiseMap = generateInternalNoiseMap<internalWidth, internalLength, internalHeight>(distribution);
        // Interpolated noise
        StealthTileMap::TileMapF<width, length, height> generatedNoiseMap;
        // 3D noise map
        int fillStartX = 0, fillStartY = 0, fillStartZ = 0;
        for (int k = 0; k < internalHeight - 1; ++k) {
            fillStartY = 0;
            for (int j = 0; j < internalLength - 1; ++j) {
                fillStartX = 0;
                for (int i = 0; i < internalWidth - 1; ++i) {
                    // 3D noise unit
                    fillCube(i, j, k, fillStartX, fillStartY, fillStartZ, internalNoiseMap, generatedNoiseMap, attenuationsX, attenuationsY, attenuationsZ);
                    fillStartX += scaleX;
                }
                fillStartY += scaleY;
            }
            fillStartZ += scaleZ;
        }
        // Return noise map.
        return generatedNoiseMap;
    }

    template <int width, int length, int height, int scaleX, int scaleY, int scaleZ, int numOctaves = 8,
        typename Distribution = std::uniform_real_distribution<float>>
    constexpr StealthTileMap::TileMapF<width, length, height> generateOctaves(Distribution&& distribution, float multiplier, float decayFactor) {
        // This multiplier should equal the last one if this is the final octave.
        if constexpr (numOctaves == 1) return (multiplier / decayFactor) * generate<width, length, height, scaleX, scaleY, scaleZ>(std::forward<Distribution&&>(distribution));
        else return multiplier * generate<width, length, height, scaleX, scaleY, scaleZ>(std::forward<Distribution&&>(distribution))
            + generateOctaves<width, length, height, ceilDivide(scaleX, 2), ceilDivide(scaleY, 2),
            ceilDivide(scaleZ, 2), numOctaves - 1>(std::forward<Distribution&&>(distribution), multiplier * decayFactor, decayFactor);
    }

    // Convenience overloads
    template <int width, int length, int height, int scaleX, int scaleY, int scaleZ, int numOctaves = 8,
        typename Distribution = std::uniform_real_distribution<float>>
    constexpr StealthTileMap::TileMapF<width, length, height> generateOctaves(Distribution&& distribution, float multiplier = 0.5f) {
        return generateOctaves<width, length, height, scaleX, scaleY, scaleZ, numOctaves>
            (std::forward<Distribution&&>(distribution), multiplier, findDecayFactor(multiplier));
    }

    template <int width, int length, int height, int scaleX, int scaleY, int scaleZ, int numOctaves = 8,
        typename Distribution = std::uniform_real_distribution<float>>
    constexpr StealthTileMap::TileMapF<width, length, height> generateOctaves(float multiplier = 0.5f) {
        return generateOctaves<width, length, height, scaleX, scaleY, scaleZ, numOctaves>
            (std::uniform_real_distribution{0.0f, 1.0f}, multiplier, findDecayFactor(multiplier));
    }

    template <int width, int length, int height, int scaleX, int scaleY, int scaleZ, int numOctaves = 8,
        typename Distribution = std::uniform_real_distribution<float>>
    constexpr StealthTileMap::TileMapF<width, length, height> generateOctaves(float multiplier, float decayFactor) {
        return generateOctaves<width, length, height, scaleX, scaleY, scaleZ, numOctaves>
            (std::uniform_real_distribution{0.0f, 1.0f}, multiplier, decayFactor);
    }
} /* StealthNoiseGenerator */

#endif /* end of include guard: NOISE_GENERATOR_3D_H */
