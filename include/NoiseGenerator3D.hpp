#ifndef NOISE_GENERATOR_3D_H
#define NOISE_GENERATOR_3D_H
#include "NoiseGenerator2D.hpp"
#include "InternalCaches.hpp"
#include <Stealth/Tensor3>
#include <Stealth/util>
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

        template <int width, int length, int height, typename overwrite, int scaleX,
            int scaleY, int scaleZ, typename InternalNoiseType, typename GeneratedNoiseType>
        constexpr void fillCube(int internalX, int internalY, int internalZ, int fillStartX, int fillStartY, int fillStartZ,
            const InternalNoiseType& internalNoiseMap, GeneratedNoiseType& generatedNoiseMap,
            const Stealth::Math::Tensor3F<scaleX>& attenuationsX, const Stealth::Math::Tensor3F<scaleY>& attenuationsY,
            const Stealth::Math::Tensor3F<scaleZ>& attenuationsZ, float multiplier = 1.0f) {
            // Only fill the part of the tile that is valid.
            const int maxValidX = std::min(width - fillStartX, scaleX);
            const int maxValidY = std::min(length - fillStartY, scaleY);
            const int maxValidZ = std::min(height - fillStartZ, scaleZ);
            // Cache noise indices
            constexpr int internalWidth = Stealth::ceilDivide(width, scaleX) + 1;
            constexpr int internalLength = Stealth::ceilDivide(length, scaleY) + 1;
            constexpr int internalArea = internalWidth * internalLength;
            const int topLeft0Index = internalX + internalY * internalWidth + internalZ * internalArea;
            const int bottomLeft0Index = topLeft0Index + internalWidth;
            const int topLeft1Index = topLeft0Index + internalArea;
            const int bottomLeft1Index = bottomLeft0Index + internalArea;
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
                float attenuationZ = attenuationsZ(k);
                for (int j = 0; j < maxValidY; ++j) {
                    float attenuationY = attenuationsY(j);
                    for (int i = 0; i < maxValidX; ++i) {
                        float attenuationX = attenuationsX(i);
                        // Interpolate based on the 8 surrounding internal noise points.
                        if constexpr (overwrite::value) {
                            generatedNoiseMap(index++) = interpolate3D(topLeft0, topRight0, bottomLeft0, bottomRight0,
                                topLeft1, topRight1, bottomLeft1, bottomRight1, attenuationX, attenuationY, attenuationZ);
                        } else {
                            generatedNoiseMap(index++) += interpolate3D(topLeft0, topRight0, bottomLeft0, bottomRight0,
                                topLeft1, topRight1, bottomLeft1, bottomRight1, attenuationX, attenuationY, attenuationZ) * multiplier;
                        }
                    }
                    // Wrap around to the first element of the next row.
                    index += generatedNoiseMap.width() - maxValidX;
                }
                // Wrap around to the first element of the next tile
                index += generatedNoiseMap.area() - maxValidY * generatedNoiseMap.width();
            }
        }
    } /* Anonymous namespace */

    template <int width, int length, int height, int scaleX, int scaleY, int scaleZ, typename overwrite
        = std::true_type, typename Distribution = decltype(DefaultDistribution), typename GeneratedNoiseType>
    constexpr GeneratedNoiseType& generate(GeneratedNoiseType& generatedNoiseMap, Distribution&& distribution
        = std::forward<Distribution&&>(DefaultDistribution), long seed = Stealth::getCurrentTime(), float multiplier = 1.0f) {
        // Generate 2D noise if there are only 2 dimensions.
        if constexpr (height == 1) {
            return generate<width, length, scaleX, scaleY, overwrite>(generatedNoiseMap,
                std::forward<Distribution&&>(distribution), seed, multiplier);
        } else if constexpr (length == 1) {
            return generate<width, height, scaleX, scaleZ, overwrite>(generatedNoiseMap,
                std::forward<Distribution&&>(distribution), seed, multiplier);
        } else if constexpr (width == 1) {
            return generate<length, height, scaleY, scaleZ, overwrite>(generatedNoiseMap,
                std::forward<Distribution&&>(distribution), seed, multiplier);
        }
        // TODO: Optimize even if scale is 1 in just a single dimension
        // if constexpr (scaleX == 1 && scaleY == 1 && scaleZ == 1) {
        //     if constexpr (overwrite::value) generatedNoiseMap.randomize(std::forward<Distribution&&>(distribution), seed);
        //     else generatedNoiseMap += GeneratedNoiseType::Random(std::forward<Distribution&&>(distribution),
        //             seed, std::default_random_engine{}) * multiplier;
        //     return generatedNoiseMap;
        // }
        // Get attenuation information
        const auto& attenuationsX{AttenuationsCache<scaleX>};
        const auto& attenuationsY{AttenuationsCache<scaleY>};
        const auto& attenuationsZ{AttenuationsCache<scaleZ>};
        // Generate a new internal noise map.
        constexpr int internalWidth = Stealth::ceilDivide(width, scaleX) + 1;
        constexpr int internalLength = Stealth::ceilDivide(length, scaleY) + 1;
        constexpr int internalHeight = Stealth::ceilDivide(height, scaleZ) + 1;
        const auto& internalNoiseMap{generateInternalNoiseMap<internalWidth, internalLength, internalHeight>
            (seed, std::forward<Distribution&&>(distribution))};
        // 3D noise map
        int fillStartX = 0, fillStartY = 0, fillStartZ = 0;
        for (int k = 0; k < internalHeight - 1; ++k) {
            fillStartY = 0;
            for (int j = 0; j < internalLength - 1; ++j) {
                fillStartX = 0;
                for (int i = 0; i < internalWidth - 1; ++i) {
                    // 3D noise unit
                    fillCube<width, length, height, overwrite>(i, j, k, fillStartX, fillStartY, fillStartZ, internalNoiseMap, generatedNoiseMap,
                        attenuationsX, attenuationsY, attenuationsZ, multiplier);
                    fillStartX += scaleX;
                }
                fillStartY += scaleY;
            }
            fillStartZ += scaleZ;
        }
        // Return noise map.
        return generatedNoiseMap;
    }

    // Return a normalization factor
    template <int width, int length, int height, int scaleX, int scaleY, int scaleZ, int numOctaves = 6,
        typename Distribution, typename GeneratedNoiseType>
    constexpr float generateOctaves3D_impl(GeneratedNoiseType& generatedNoiseMap, long seed,
        Distribution&& distribution, float decayFactor, float accumulator = 1.0f) {
        // First generate this layer...
        generate<width, length, height, scaleX, scaleY, scaleZ, std::false_type>(generatedNoiseMap,
            std::forward<Distribution&&>(distribution), seed, accumulator);
        // ...then generate the next octaves.
        if constexpr (numOctaves > 1) {
            return accumulator + generateOctaves3D_impl<width, length, height, Stealth::ceilDivide(scaleX, 2),
                Stealth::ceilDivide(scaleY, 2), Stealth::ceilDivide(scaleZ, 2), numOctaves - 1>(generatedNoiseMap, seed,
                std::forward<Distribution&&>(distribution), decayFactor, accumulator * decayFactor);
        } else {
            return accumulator;
        }
    }

    // Convenience overloads
    template <int width, int length, int height, int scaleX, int scaleY, int scaleZ, int numOctaves = 6,
        typename overwrite = std::true_type, typename Distribution = decltype(DefaultDistribution), typename GeneratedNoiseType>
    constexpr GeneratedNoiseType& generateOctaves(GeneratedNoiseType& generatedNoiseMap, Distribution&& distribution
        = std::forward<Distribution&&>(DefaultDistribution), long seed = Stealth::getCurrentTime(), float decayFactor = 0.5f) {
        // Zero if we need to overwrite
        if constexpr (overwrite::value) generatedNoiseMap = 0.0f;
        // Generate and normalize!
        generatedNoiseMap /= generateOctaves3D_impl<width, length, height, scaleX, scaleY, scaleZ, numOctaves>
            (generatedNoiseMap, seed, std::forward<Distribution&&>(distribution), decayFactor);
        return generatedNoiseMap;
    }
} /* StealthNoiseGenerator */

#endif /* end of include guard: NOISE_GENERATOR_3D_H */
