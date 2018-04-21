#ifndef NOISE_GENERATOR_2D_H
#define NOISE_GENERATOR_2D_H
#include "NoiseGenerator1D.hpp"
#include "Internal.hpp"
#include <Stealth/Tensor3>
#include <Stealth/util>
#include <Stealth/time>
#include <random>

namespace Stealth::Noise {
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

        // Interpolate a square section among 4 corners.
        template <int width, int length, typename overwrite, int scaleX, int scaleY,
            typename InternalNoiseType, typename GeneratedNoiseType>
        constexpr void fillSquare(int internalX, int internalY, int fillStartX, int fillStartY,
            const InternalNoiseType& internalNoiseMap, GeneratedNoiseType& generatedNoiseMap,
            const Stealth::Tensor::Tensor3F<scaleX>& attenuationsX, const Stealth::Tensor::Tensor3F<scaleY>& attenuationsY,
            float multiplier = 1.0f) {
            // Only fill the part of the tile that is valid.
            const int maxValidX = std::min(width - fillStartX, scaleX);
            const int maxValidY = std::min(length - fillStartY, scaleY);
            // Cache noise indices
            constexpr int internalWidth = Stealth::ceilDivide(width, scaleX) + 1;
            const int topLeftIndex = internalX + internalY * internalWidth;
            const int bottomLeftIndex = topLeftIndex + internalWidth;
            // Cache noise values
            float topLeft = internalNoiseMap(topLeftIndex);
            float topRight = internalNoiseMap(topLeftIndex + 1);
            float bottomLeft = internalNoiseMap(bottomLeftIndex);
            float bottomRight = internalNoiseMap(bottomLeftIndex + 1);
            // Loop over one interpolation kernel tile.
            int index = fillStartX + fillStartY * generatedNoiseMap.width();
            for (int j = 0; j < maxValidY; ++j) {
                float attenuationY = attenuationsY(j);
                for (int i = 0; i < maxValidX; ++i) {
                    float attenuationX = attenuationsX(i);
                    // Interpolate based on the 4 surrounding internal noise points.
                    if constexpr (overwrite::value) {
                        generatedNoiseMap(index++) = interpolate2D(topLeft, topRight, bottomLeft,
                            bottomRight, attenuationX, attenuationY);
                    } else {
                        generatedNoiseMap(index++) += interpolate2D(topLeft, topRight, bottomLeft,
                            bottomRight, attenuationX, attenuationY) * multiplier;
                    }
                }
                // Wrap around to the first element of the next row.
                index += generatedNoiseMap.width() - maxValidX;
            }
        }
    } /* Anonymous namespace */

    template <int width, int length, int scaleX, int scaleY, typename overwrite
        = std::true_type, typename Distribution, typename GeneratedNoiseType>
    constexpr GeneratedNoiseType& generate(GeneratedNoiseType& generatedNoiseMap, Distribution&& distribution
        = DefaultDistribution{0.f, 1.f}, long seed = Stealth::getCurrentTime(),
        float multiplier = 1.0f) {
        // Generate 1D noise if there's only 1 dimension.
        if constexpr (length == 1) {
            return generate<width, scaleX, overwrite>(generatedNoiseMap,
                std::forward<Distribution&&>(distribution), seed, multiplier);
        } else if constexpr (width == 1) {
            return generate<length, scaleY, overwrite>(generatedNoiseMap,
                std::forward<Distribution&&>(distribution), seed, multiplier);
        }
        // Get attenuation information
        const auto attenuationsX{generateAttenuations<scaleX>()};
        const auto attenuationsY{generateAttenuations<scaleY>()};
        // Generate a new internal noise map.
        constexpr int internalWidth = Stealth::ceilDivide(width, scaleX) + 1;
        constexpr int internalLength = Stealth::ceilDivide(length, scaleY) + 1;
        const auto internalNoiseMap{generateInternalNoiseMap<internalWidth, internalLength>
            (seed, std::forward<Distribution&&>(distribution))};
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

    // Return a normalization factor and generate the noisemap in-place.
    template <int width, int length, int scaleX, int scaleY, int numOctaves = 6, typename overwrite, typename Distribution, typename GeneratedNoiseType>
    constexpr float generateOctaves2D_impl(GeneratedNoiseType& generatedNoiseMap, long seed,
        Distribution&& distribution, float decayFactor, float accumulator = 1.0f) {
        // First generate this layer...
        generate<width, length, scaleX, scaleY, overwrite>(generatedNoiseMap,
            std::forward<Distribution&&>(distribution), seed, accumulator);
        // ...then generate the next octaves.
        if constexpr (numOctaves > 1) {
            return accumulator + generateOctaves2D_impl<width, length, Stealth::ceilDivide(scaleX, 2),
                Stealth::ceilDivide(scaleY, 2), numOctaves - 1, std::false_type>(generatedNoiseMap, seed, std::forward<Distribution&&>(distribution),
                decayFactor, accumulator * decayFactor);
        } else {
            return accumulator;
        }
    }

    // Convenience overloads
    template <int width, int length, int scaleX, int scaleY, int numOctaves = 6, typename overwrite
        = std::true_type, typename Distribution, typename GeneratedNoiseType>
    constexpr GeneratedNoiseType& generateOctaves(GeneratedNoiseType& generatedNoiseMap, Distribution&& distribution
        = DefaultDistribution{0.f, 1.f}, long seed = Stealth::getCurrentTime(), float decayFactor = 0.5f) {
        // Generate and normalize!
        generatedNoiseMap /= generateOctaves2D_impl<width, length, scaleX, scaleY, numOctaves, overwrite>(generatedNoiseMap,
            seed, std::forward<Distribution&&>(distribution), decayFactor);
        return generatedNoiseMap;
    }
} /* Stealth::Noise */

#endif /* end of include guard: NOISE_GENERATOR_2D_H */
