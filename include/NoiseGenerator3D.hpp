#ifndef NOISE_GENERATOR_3D_H
#define NOISE_GENERATOR_3D_H
#include "NoiseGenerator2D.hpp"
#include "Internal.hpp"
#include <Tensor3>
#include <random>

namespace Stealth::Noise {
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

        // Interpolate a cubic section among 8 corners.
        template <int width, int length, int height, typename overwrite, int scaleX,
            int scaleY, int scaleZ, typename InternalNoiseType, typename GeneratedNoiseType>
        constexpr void fillCube(int internalX, int internalY, int internalZ, int fillStartX, int fillStartY, int fillStartZ,
            const InternalNoiseType& internalNoiseMap, GeneratedNoiseType& generatedNoiseMap,
            const Stealth::Tensor::Tensor3F<scaleX>& attenuationsX, const Stealth::Tensor::Tensor3F<scaleY>& attenuationsY,
            const Stealth::Tensor::Tensor3F<scaleZ>& attenuationsZ, float multiplier = 1.0f) {
            // Only fill the part of the tile that is valid.
            const int maxValidX = std::min(width - fillStartX, scaleX);
            const int maxValidY = std::min(length - fillStartY, scaleY);
            const int maxValidZ = std::min(height - fillStartZ, scaleZ);
            // Cache noise indices
            constexpr int internalWidth = ceilDivide(width, scaleX) + 1;
            constexpr int internalLength = ceilDivide(length, scaleY) + 1;
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
        = std::true_type, typename Distribution, typename GeneratedNoiseType>
    constexpr GeneratedNoiseType& generate(GeneratedNoiseType& generatedNoiseMap, Distribution&& distribution
        = DefaultDistribution{0.f, 1.f}, long seed = 0, float multiplier = 1.0f) {
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
        // Get attenuation information
        const auto attenuationsX{generateAttenuations<scaleX>()};
        const auto attenuationsY{generateAttenuations<scaleY>()};
        const auto attenuationsZ{generateAttenuations<scaleZ>()};
        // Generate a new internal noise map.
        constexpr int internalWidth = ceilDivide(width, scaleX) + 1;
        constexpr int internalLength = ceilDivide(length, scaleY) + 1;
        constexpr int internalHeight = ceilDivide(height, scaleZ) + 1;
        const auto internalNoiseMap{generateInternalNoiseMap<internalWidth, internalLength, internalHeight>
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

    // Return a normalization factor and generate the noisemap in-place.
    template <int width, int length, int height, int scaleX, int scaleY, int scaleZ, int numOctaves = 6,
        typename overwrite, typename Distribution, typename GeneratedNoiseType>
    constexpr float generateOctaves3D_impl(GeneratedNoiseType& generatedNoiseMap, long seed,
        Distribution&& distribution, float decayFactor, float accumulator = 1.0f) {
        // First generate this layer...
        generate<width, length, height, scaleX, scaleY, scaleZ, overwrite>(generatedNoiseMap,
            std::forward<Distribution&&>(distribution), seed, accumulator);
        // ...then generate the next octaves.
        if constexpr (numOctaves > 1) {
            return accumulator + generateOctaves3D_impl<width, length, height, ceilDivide(scaleX, 2),
                ceilDivide(scaleY, 2), ceilDivide(scaleZ, 2), numOctaves - 1, std::false_type>
                (generatedNoiseMap, seed, std::forward<Distribution&&>(distribution), decayFactor, accumulator * decayFactor);
        } else {
            return accumulator;
        }
    }

    // Convenience overloads
    template <int width, int length, int height, int scaleX, int scaleY, int scaleZ, int numOctaves = 6,
        typename overwrite = std::true_type, typename Distribution, typename GeneratedNoiseType>
    constexpr GeneratedNoiseType& generateOctaves(GeneratedNoiseType& generatedNoiseMap, Distribution&& distribution
        = DefaultDistribution{0.f, 1.f}, long seed = 0, float decayFactor = 0.5f) {
        // Generate and normalize!
        generatedNoiseMap /= generateOctaves3D_impl<width, length, height, scaleX, scaleY, scaleZ, numOctaves, overwrite>
            (generatedNoiseMap, seed, std::forward<Distribution&&>(distribution), decayFactor);
        return generatedNoiseMap;
    }
} /* Stealth::Noise */

#endif /* end of include guard: NOISE_GENERATOR_3D_H */
