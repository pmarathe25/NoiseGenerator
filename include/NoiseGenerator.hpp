#ifndef NOISE_GENERATOR_H
#define NOISE_GENERATOR_H
#define CURRENT_TIME std::chrono::system_clock::now().time_since_epoch().count()
#include "InterpolationKernel.hpp"
#include "TileMap/TileMap.hpp"
#include <stealthutil>
#include <unordered_map>
#include <chrono>
#include <random>

namespace StealthWorldGenerator {
    namespace {
        // Maintain a cache of interpolation kernels of different sizes
        static inline std::unordered_map<int, std::unique_ptr<const InterpolationKernelBase>> kernels{};
    }
    class NoiseGenerator {
        public:
            // Create octaved noise
            template <int width, int length = 1, int height = 1, int scale = 1, int numOctaves = 8,
                typename Distribution = std::uniform_real_distribution<float>, typename Generator = std::default_random_engine>
            constexpr StealthTileMap::TileMapF<width, length, height> generateOctaves(Distribution distribution,
                Generator generator, float multiplier, float decayFactor) {
                if constexpr (numOctaves == 1) {
                    // This multiplier should equal the last one if this is the final octave.
                    return (multiplier / decayFactor) * generate<width, length, height, scale>(distribution, generator);
                } else {
                    return multiplier * generate<width, length, height, scale>(distribution, generator)
                        + generateOctaves<width, length, height, ceilDivide(scale, 2), numOctaves - 1>(distribution,
                        generator, multiplier * decayFactor, decayFactor);
                }
            }

            // Overloads for octaved noise
            template <int width, int length = 1, int height = 1, int scale = 1, int numOctaves = 8,
                typename Distribution = std::uniform_real_distribution<float>, typename Generator = std::default_random_engine>
            constexpr StealthTileMap::TileMapF<width, length, height> generateOctaves(Distribution distribution = std::uniform_real_distribution(0.0f, 1.0f),
                Generator generator = std::default_random_engine(CURRENT_TIME), float multiplier = 0.5f) {
                // Automatically determine decayFactor if it is not provided
                // Treat as an infinite geometric sum where multiplier = alpha and decayFactor = r
                float decayFactor = (1.0f - multiplier <= 0.0f) ? 0.0001f : 1.0f - multiplier;
                return generateOctaves<width, length, height, scale, numOctaves>(distribution, generator, multiplier, decayFactor);
            }

            template <int width, int length = 1, int height = 1, int scale = 1, int numOctaves = 8>
            constexpr StealthTileMap::TileMapF<width, length, height> generateOctaves(float multiplier, float decayFactor) {
                return generateOctaves<width, length, height, scale, numOctaves>(std::uniform_real_distribution(0.0f, 1.0f),
                    std::default_random_engine(CURRENT_TIME), multiplier, decayFactor);
            }

            template <int width, int length = 1, int height = 1, int scale = 1, int numOctaves = 8>
            constexpr StealthTileMap::TileMapF<width, length, height> generateOctaves(float multiplier) {
                float decayFactor = (1.0f - multiplier <= 0.0f) ? 0.0001f : 1.0f - multiplier;
                return generateOctaves<width, length, height, scale, numOctaves>(std::uniform_real_distribution(0.0f, 1.0f),
                    std::default_random_engine(CURRENT_TIME), multiplier, decayFactor);
            }

            // Create the smoothed noise
            template <int width, int length = 1, int height = 1, int scale = 1, typename Distribution
                = std::uniform_real_distribution<float>, typename Generator = std::default_random_engine>
            constexpr StealthTileMap::TileMapF<width, length, height> generate(Distribution distribution
                = std::uniform_real_distribution(0.0f, 1.0f), Generator generator = std::default_random_engine(CURRENT_TIME)) {
                // Generate a new interpolation kernel if one does not exist.
                if (kernels.count(scale) < 1) {
                    kernels.emplace(scale, std::make_unique<InterpolationKernel<scale>>());
                }
                const InterpolationKernel<scale>& kernel = *static_cast<const InterpolationKernel<scale>*>(kernels[scale].get());
                // Generate a new internal noise map.
                constexpr int internalWidth = ceilDivide(width, scale) + 1;
                constexpr int internalLength = (length == 1) ? 1 : ceilDivide(length, scale) + 1;
                constexpr int internalHeight = (height == 1) ? 1 : ceilDivide(height, scale) + 1;
                // Scaled noise map
                auto internalNoiseMap = generateInternalNoiseMap<internalWidth, internalLength, internalHeight>(distribution, generator);
                // Interpolated noise
                StealthTileMap::TileMapF<width, length, height> generatedNoise;
                generateNoiseMap(internalNoiseMap, generatedNoise, kernel);
                // Return noise map.
                return generatedNoise;
            }
        private:
            constexpr float interpolate1D(float left, float right, float attenuation) noexcept {
                // Interpolate between two points
                float nx = left * (1.0f - attenuation) + right * attenuation;
                return nx;
            }

            constexpr float interpolate2D(float topLeft, float topRight, float bottomLeft,
                float bottomRight, float attenuationX, float attenuationY) noexcept {
                // Interpolate horizontally
                float nx0 = interpolate1D(topLeft, topRight, attenuationX);
                float nx1 = interpolate1D(bottomLeft, bottomRight, attenuationX);
                // Interpolate vertically
                float nxy = interpolate1D(nx0, nx1, attenuationY);
                return nxy;
            }

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

            // Initialize with random values according to provided distribution
            template <int internalWidth, int internalLength, int internalHeight, typename Distribution, typename Generator>
            constexpr auto generateInternalNoiseMap(Distribution& distribution, Generator& generator) {
                // Internal noise map should be large enough to fit tiles of size (scale, scale).
                StealthTileMap::TileMapF<internalWidth, internalLength, internalHeight> internalNoiseMap{};
                for (int k = 0; k < internalHeight; ++k) {
                    for (int j = 0; j < internalLength; ++j) {
                        for (int i = 0; i < internalWidth; ++i) {
                            float val = distribution(generator);
                            internalNoiseMap(i, j, k) = val;
                        }
                    }
                }
                return internalNoiseMap;
            }

            template <int scale, int internalWidth, int internalLength, int internalHeight, int width, int length, int height>
            constexpr void generateNoiseMap(const StealthTileMap::TileMapF<internalWidth, internalLength, internalHeight>& internalNoise,
                StealthTileMap::TileMapF<width, length, height>& generatedNoise, const InterpolationKernel<scale>& kernel) {
                // Loop over the internal noise map and fill sections in between
                if constexpr (internalLength == 1 && internalHeight == 1) {
                    // 1D noise map
                    int scaledX = 0;
                    for (int i = 0; i < internalWidth - 1; ++i) {
                        // 1D noise
                        fillLine(i, scaledX, &internalNoise, &generatedNoise, &kernel);
                        scaledX += scale;
                    }
                } else if constexpr (internalHeight == 1) {
                    // 2D noise map
                    int scaledX = 0, scaledY = 0;
                    for (int j = 0; j < internalLength - 1; ++j) {
                        scaledX = 0;
                        for (int i = 0; i < internalWidth - 1; ++i) {
                            // 2D noise
                            fillSquare(i, j, scaledX, scaledY, &internalNoise, &generatedNoise, &kernel);
                            scaledX += scale;
                        }
                        scaledY += scale;
                    }
                } else {
                    // 3D noise map
                }
            }

            template <int scale, int internalWidth, int width>
            constexpr void fillLine(int internalX, int scaledX, const StealthTileMap::TileMapF<internalWidth>* internalNoise,
                StealthTileMap::TileMapF<width>* generatedNoise, const InterpolationKernel<scale>* kernel) {
                // Only fill the part of the length that is valid.
                const int maxValidX = std::min(width - scaledX, scale);
                // Cache local gradient vectors
                float left = internalNoise -> operator()(internalX);
                float right = internalNoise -> operator()(internalX + 1);
                // Cache points and attenuations TileMaps
                const auto& attenuations = kernel -> getAttenuations();
                // Loop over one interpolation kernel tile.
                for (int i = 0; i < maxValidX; ++i) {
                    // Interpolate based on the 4 surrounding internal noise points.
                    generatedNoise -> operator()(scaledX + i) = interpolate1D(left, right, attenuations(i));
                }
            }

            template <int scale, int internalWidth, int internalLength, int width, int length>
            constexpr void fillSquare(int internalX, int internalY, int scaledX, int scaledY,
                const StealthTileMap::TileMapF<internalWidth, internalLength>* internalNoise,
                StealthTileMap::TileMapF<width, length>* generatedNoise, const InterpolationKernel<scale>* kernel) {
                // Only fill the part of the tile that is valid.
                const int maxValidX = std::min(width - scaledX, scale);
                const int maxValidY = std::min(length - scaledY, scale);
                // Cache local gradient vectors
                float topLeft = internalNoise -> operator()(internalX, internalY);
                float topRight = internalNoise -> operator()(internalX + 1, internalY);
                float bottomLeft = internalNoise -> operator()(internalX, internalY + 1);
                float bottomRight = internalNoise -> operator()(internalX + 1, internalY + 1);
                // Cache points and attenuations TileMaps
                const auto& attenuations = kernel -> getAttenuations();
                // Loop over one interpolation kernel tile.
                for (int j = 0; j < maxValidY; ++j) {
                    for (int i = 0; i < maxValidX; ++i) {
                        // Interpolate based on the 4 surrounding internal noise points.
                        generatedNoise -> operator()(scaledX + i, scaledY + j) = interpolate2D(topLeft,
                            topRight, bottomLeft, bottomRight, attenuations(i), attenuations(j));
                    }
                }
            }
    };
} /* StealthWorldGenerator */

#endif
