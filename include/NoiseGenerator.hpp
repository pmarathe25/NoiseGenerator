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
        static inline std::default_random_engine generator{CURRENT_TIME};
    }
    class NoiseGenerator {
        public:
            constexpr float findDecayFactor(float multiplier) {
                // Treat as an infinite geometric sum where multiplier = alpha and decayFactor = r
                return (1.0f - multiplier <= 0.0f) ? 0.0001f : 1.0f - multiplier;
            }
            // Create octaved noise
            template <int scaleX, int scaleY, int scaleZ, int numOctaves = 8, int width = 1, int length = 1, int height = 1,
                typename Distribution = std::uniform_real_distribution<float>>
            constexpr StealthTileMap::TileMapF<width, length, height> generateOctaves(Distribution distribution,
                float multiplier, float decayFactor) {
                if constexpr (numOctaves == 1) {
                    // This multiplier should equal the last one if this is the final octave.
                    return (multiplier / decayFactor) * generate<scaleX, scaleY, scaleZ, width, length, height>(distribution);
                } else {
                    return multiplier * generate<scaleX, scaleY, scaleZ, width, length, height>(distribution)
                        + generateOctaves<ceilDivide(scaleX, 2), ceilDivide(scaleY, 2), ceilDivide(scaleZ, 2),
                        numOctaves - 1, width, length, height>(distribution, multiplier * decayFactor, decayFactor);
                }
            }

            // Overloads for octaved noise
            template <int scaleX, int scaleY, int scaleZ, int numOctaves = 8, int width = 1, int length = 1, int height = 1,
                typename Distribution = std::uniform_real_distribution<float>, typename Generator = std::default_random_engine>
            constexpr StealthTileMap::TileMapF<width, length, height> generateOctaves(Distribution distribution = std::uniform_real_distribution(0.0f, 1.0f),
                float multiplier = 0.5f) {
                // Automatically determine decayFactor if it is not provided
                return generateOctaves<scaleX, scaleY, scaleZ, numOctaves, width, length, height>(distribution, multiplier, findDecayFactor(multiplier));
            }

            template <int scaleX, int scaleY, int scaleZ, int numOctaves = 8, int width = 1, int length = 1, int height = 1>
            constexpr StealthTileMap::TileMapF<width, length, height> generateOctaves(float multiplier, float decayFactor) {
                return generateOctaves<scaleX, scaleY, scaleZ, numOctaves, width, length, height>(std::uniform_real_distribution(0.0f, 1.0f),
                    multiplier, decayFactor);
            }

            template <int scaleX, int scaleY, int scaleZ, int numOctaves = 8, int width = 1, int length = 1, int height = 1>
            constexpr StealthTileMap::TileMapF<width, length, height> generateOctaves(float multiplier) {
                return generateOctaves<scaleX, scaleY, scaleZ, numOctaves, width, length, height>(std::uniform_real_distribution(0.0f, 1.0f),
                    multiplier, findDecayFactor(multiplier));
            }

            // Create the smoothed noise
            template <int scaleX, int scaleY, int scaleZ, int width = 1, int length = 1, int height = 1, typename Distribution
                = std::uniform_real_distribution<float>>
            constexpr StealthTileMap::TileMapF<width, length, height> generate(Distribution distribution
                = std::uniform_real_distribution(0.0f, 1.0f)) {
                // Generate new interpolation kernels if they do not exist.
                if (kernels.count(scaleX) < 1) {
                    kernels.emplace(scaleX, std::make_unique<InterpolationKernel<scaleX>>());
                }
                if (kernels.count(scaleY) < 1) {
                    kernels.emplace(scaleY, std::make_unique<InterpolationKernel<scaleY>>());
                }
                if (kernels.count(scaleZ) < 1) {
                    kernels.emplace(scaleZ, std::make_unique<InterpolationKernel<scaleZ>>());
                }
                const InterpolationKernel<scaleX>& kernelX = *static_cast<const InterpolationKernel<scaleX>*>(kernels[scaleX].get());
                const InterpolationKernel<scaleY>& kernelY = *static_cast<const InterpolationKernel<scaleY>*>(kernels[scaleY].get());
                const InterpolationKernel<scaleZ>& kernelZ = *static_cast<const InterpolationKernel<scaleZ>*>(kernels[scaleZ].get());
                // Generate a new internal noise map.
                constexpr int internalWidth = ceilDivide(width, scaleX) + 1;
                constexpr int internalLength = (length == 1) ? 1 : ceilDivide(length, scaleY) + 1;
                constexpr int internalHeight = (height == 1) ? 1 : ceilDivide(height, scaleZ) + 1;
                // Scaled noise map
                auto internalNoiseMap = generateInternalNoiseMap<internalWidth, internalLength, internalHeight>(distribution);
                // Interpolated noise
                StealthTileMap::TileMapF<width, length, height> generatedNoise;
                generateNoiseMap(internalNoiseMap, generatedNoise, kernelX, kernelY, kernelZ);
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
            template <int internalWidth, int internalLength, int internalHeight, typename Distribution>
            constexpr auto generateInternalNoiseMap(Distribution& distribution) {
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

            template <int scaleX, int scaleY, int scaleZ, int internalWidth, int internalLength, int internalHeight, int width, int length, int height>
            constexpr void generateNoiseMap(const StealthTileMap::TileMapF<internalWidth, internalLength, internalHeight>& internalNoise,
                StealthTileMap::TileMapF<width, length, height>& generatedNoise, const InterpolationKernel<scaleX>& kernelX,
                const InterpolationKernel<scaleY>& kernelY, const InterpolationKernel<scaleZ>& kernelZ) {
                // Loop over the internal noise map and fill sections in between
                if constexpr (internalLength == 1 && internalHeight == 1) {
                    // 1D noise map
                    int fillStartX = 0;
                    for (int i = 0; i < internalWidth - 1; ++i) {
                        // 1D noise
                        fillLine(i, fillStartX, internalNoise, generatedNoise, kernelX);
                        fillStartX += scaleX;
                    }
                } else if constexpr (internalHeight == 1) {
                    // 2D noise map
                    int fillStartX = 0, fillStartY = 0;
                    for (int j = 0; j < internalLength - 1; ++j) {
                        fillStartX = 0;
                        for (int i = 0; i < internalWidth - 1; ++i) {
                            // 2D noise
                            fillSquare(i, j, fillStartX, fillStartY, internalNoise, generatedNoise, kernelX, kernelY);
                            fillStartX += scaleX;
                        }
                        fillStartY += scaleY;
                    }
                } else {
                    // 3D noise map
                }
            }

            template <int scaleX, int internalWidth, int width>
            constexpr void fillLine(int internalX, int fillStartX, const StealthTileMap::TileMapF<internalWidth>& internalNoise,
                StealthTileMap::TileMapF<width>& generatedNoise, const InterpolationKernel<scaleX>& kernelX) {
                // Only fill the part of the length that is valid.
                const int maxValidX = std::min(width - fillStartX, scaleX);
                // Cache local gradient vectors
                float left = internalNoise(internalX);
                float right = internalNoise(internalX + 1);
                // Cache points and attenuations TileMaps
                const auto& attenuationsX = kernelX.getAttenuations();
                // Loop over one interpolation kernel tile.
                for (int i = 0; i < maxValidX; ++i) {
                    // Interpolate based on the 4 surrounding internal noise points.
                    generatedNoise(fillStartX + i) = interpolate1D(left, right, attenuationsX(i));
                }
            }

            template <int scaleX, int scaleY, int internalWidth, int internalLength, int width, int length>
            constexpr void fillSquare(int internalX, int internalY, int fillStartX, int fillStartY,
                const StealthTileMap::TileMapF<internalWidth, internalLength>& internalNoise,
                StealthTileMap::TileMapF<width, length>& generatedNoise, const InterpolationKernel<scaleX>& kernelX,
                const InterpolationKernel<scaleY>& kernelY) {
                // Only fill the part of the tile that is valid.
                const int maxValidX = std::min(width - fillStartX, scaleX);
                const int maxValidY = std::min(length - fillStartY, scaleY);
                // Cache local gradient vectors
                float topLeft = internalNoise(internalX, internalY);
                float topRight = internalNoise(internalX + 1, internalY);
                float bottomLeft = internalNoise(internalX, internalY + 1);
                float bottomRight = internalNoise(internalX + 1, internalY + 1);
                // Cache points and attenuations TileMaps
                const auto& attenuationsX = kernelX.getAttenuations();
                const auto& attenuationsY = kernelY.getAttenuations();
                // Loop over one interpolation kernel tile.
                for (int j = 0; j < maxValidY; ++j) {
                    for (int i = 0; i < maxValidX; ++i) {
                        // Interpolate based on the 4 surrounding internal noise points.
                        generatedNoise(fillStartX + i, fillStartY + j) = interpolate2D(topLeft,
                            topRight, bottomLeft, bottomRight, attenuationsX(i), attenuationsY(j));
                    }
                }
            }
    };
} /* StealthWorldGenerator */

#endif
