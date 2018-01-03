#ifndef NOISE_GENERATOR_H
#define NOISE_GENERATOR_H
#define CURRENT_TIME std::chrono::system_clock::now().time_since_epoch().count()
#include "InterpolationKernel.hpp"
#include "TileMap/TileMap.hpp"
#include <stealthutil>
#include <unordered_map>
#include <chrono>
#include <random>

namespace StealthNoiseGenerator {
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

            // Create 1D smoothed noise
            template <int width, int scaleX, typename Distribution = std::uniform_real_distribution<float>>
            constexpr StealthTileMap::TileMapF<width> generate(Distribution distribution = std::uniform_real_distribution(0.0f, 1.0f)) {
                    // Generate new interpolation kernels if they do not exist.
                    if (kernels.count(scaleX) < 1) kernels.emplace(scaleX, std::make_unique<InterpolationKernel<scaleX>>());
                    // Get kernel
                    const InterpolationKernel<scaleX>& kernelX = *static_cast<const InterpolationKernel<scaleX>*>(kernels[scaleX].get());
                    // Generate a new internal noise map.
                    constexpr int internalWidth = ceilDivide(width, scaleX) + 1;
                    // Scaled noise map
                    auto internalNoiseMap = generateInternalNoiseMap<internalWidth>(distribution);
                    // Interpolated noise
                    StealthTileMap::TileMapF<width> generatedNoiseMap;
                    // 1D noise map
                    int fillStartX = 0;
                    for (int i = 0; i < internalWidth - 1; ++i) {
                        // 1D noise
                        fillLine(i, fillStartX, internalNoiseMap, generatedNoiseMap, kernelX);
                        fillStartX += scaleX;
                    }
                    // Return noise map.
                    return generatedNoiseMap;
            }

            // Create 1D octaved noise
            template <int width, int scaleX, int numOctaves = 8, typename Distribution = std::uniform_real_distribution<float>>
            constexpr StealthTileMap::TileMapF<width> generateOctaves(Distribution distribution = std::uniform_real_distribution(0.0f, 1.0f),
                float multiplier = 0.5f, float decayFactor = 0.5f) {
                // This multiplier should equal the last one if this is the final octave.
                if constexpr (numOctaves == 1) return (multiplier / decayFactor) * generate<width, scaleX>(distribution);
                else return multiplier * generate<width, scaleX>(distribution) + generateOctaves<width, ceilDivide(scaleX, 2),
                    numOctaves - 1>(distribution, multiplier * decayFactor, decayFactor);
            }

            // Create 2D smoothed noise
            template <int width, int length, int scaleX, int scaleY, typename Distribution = std::uniform_real_distribution<float>>
            constexpr StealthTileMap::TileMapF<width, length> generate(Distribution distribution
                = std::uniform_real_distribution(0.0f, 1.0f)) {
                    // Generate new interpolation kernels if they do not exist.
                    if (kernels.count(scaleX) < 1) kernels.emplace(scaleX, std::make_unique<InterpolationKernel<scaleX>>());
                    if (kernels.count(scaleY) < 1) kernels.emplace(scaleY, std::make_unique<InterpolationKernel<scaleY>>());
                    // Get kernels
                    const InterpolationKernel<scaleX>& kernelX = *static_cast<const InterpolationKernel<scaleX>*>(kernels[scaleX].get());
                    const InterpolationKernel<scaleY>& kernelY = *static_cast<const InterpolationKernel<scaleY>*>(kernels[scaleY].get());
                    // Generate a new internal noise map.
                    constexpr int internalWidth = ceilDivide(width, scaleX) + 1;
                    constexpr int internalLength = ceilDivide(length, scaleY) + 1;
                    // Scaled noise map
                    auto internalNoiseMap = generateInternalNoiseMap<internalWidth, internalLength>(distribution);
                    // Interpolated noise
                    StealthTileMap::TileMapF<width, length> generatedNoiseMap;
                    // 2D noise map
                    int fillStartX = 0, fillStartY = 0;
                    for (int j = 0; j < internalLength - 1; ++j) {
                        fillStartX = 0;
                        for (int i = 0; i < internalWidth - 1; ++i) {
                            // 2D noise
                            fillSquare(i, j, fillStartX, fillStartY, internalNoiseMap, generatedNoiseMap, kernelX, kernelY);
                            fillStartX += scaleX;
                        }
                        fillStartY += scaleY;
                    }
                    // Return noise map.
                    return generatedNoiseMap;
            }

            // Create 2D octaved noise
            template <int width, int length, int scaleX, int scaleY, int numOctaves = 8, typename Distribution = std::uniform_real_distribution<float>>
            constexpr StealthTileMap::TileMapF<width, length> generateOctaves(Distribution distribution = std::uniform_real_distribution(0.0f, 1.0f),
                float multiplier = 0.5f, float decayFactor = 0.5f) {
                // This multiplier should equal the last one if this is the final octave.
                if constexpr (numOctaves == 1) return (multiplier / decayFactor) * generate<width, length, scaleX, scaleY>(distribution);
                else return multiplier * generate<width, length, scaleX, scaleY>(distribution)
                    + generateOctaves<width, length, ceilDivide(scaleX, 2), ceilDivide(scaleY, 2),
                    numOctaves - 1>(distribution, multiplier * decayFactor, decayFactor);
            }

            // Create 3D smoothed noise
            // template <int width, int length, int height, int scaleX, int scaleY, int scaleZ, typename Distribution
            //     = std::uniform_real_distribution<float>>
            // constexpr StealthTileMap::TileMapF<width, length, height> generate(Distribution distribution
            //     = std::uniform_real_distribution(0.0f, 1.0f)) {
            //     // Generate new interpolation kernels if they do not exist.
            //     if (kernels.count(scaleX) < 1) kernels.emplace(scaleX, std::make_unique<InterpolationKernel<scaleX>>());
            //     if (kernels.count(scaleY) < 1) kernels.emplace(scaleY, std::make_unique<InterpolationKernel<scaleY>>());
            //     if (kernels.count(scaleZ) < 1) kernels.emplace(scaleZ, std::make_unique<InterpolationKernel<scaleZ>>());
            //     const InterpolationKernel<scaleX>& kernelX = *static_cast<const InterpolationKernel<scaleX>*>(kernels[scaleX].get());
            //     const InterpolationKernel<scaleY>& kernelY = *static_cast<const InterpolationKernel<scaleY>*>(kernels[scaleY].get());
            //     const InterpolationKernel<scaleZ>& kernelZ = *static_cast<const InterpolationKernel<scaleZ>*>(kernels[scaleZ].get());
            //     // Generate a new internal noise map.
            //     constexpr int internalWidth = ceilDivide(width, scaleX) + 1;
            //     constexpr int internalLength = ceilDivide(length, scaleY) + 1;
            //     constexpr int internalHeight = ceilDivide(height, scaleZ) + 1;
            //     // Scaled noise map
            //     auto internalNoiseMap = generateInternalNoiseMap<internalWidth, internalLength, internalHeight>(distribution);
            //     // Interpolated noise
            //     StealthTileMap::TileMapF<width, length, height> generatedNoiseMap;
            //
            //     // TODO: Write FillCube function and complete 3D generation
            //
            //     // Return noise map.
            //     return generatedNoiseMap;
            // }

            // Create 3D octaved noise
            // template <int width, int length, int height int scaleX, int scaleY, int scaleZ, int numOctaves = 8,
            //     typename Distribution = std::uniform_real_distribution<float>>
            // constexpr StealthTileMap::TileMapF<width, length, height> generateOctaves(Distribution distribution = std::uniform_real_distribution(0.0f, 1.0f),
            //     float multiplier = 0.5f, float decayFactor = 0.5f) {
            //     // This multiplier should equal the last one if this is the final octave.
            //     if constexpr (numOctaves == 1) return (multiplier / decayFactor) * generate<width, length, height, scaleX, scaleY, scaleZ>(distribution);
            //     else return multiplier * generate<width, length, height, scaleX, scaleY, scaleZ>(distribution)
            //         + generateOctaves<width, length, height, ceilDivide(scaleX, 2), ceilDivide(scaleY, 2),
            //         ceilDivide(scaleZ, 2), numOctaves - 1>(distribution, multiplier * decayFactor, decayFactor);
            // }

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
            template <int internalWidth, int internalLength = 1, int internalHeight = 1, typename Distribution>
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

            template <int scaleX, int internalWidth, int width>
            constexpr void fillLine(int internalX, int fillStartX, const StealthTileMap::TileMapF<internalWidth>& internalNoiseMap,
                StealthTileMap::TileMapF<width>& generatedNoiseMap, const InterpolationKernel<scaleX>& kernelX) {
                // Only fill the part of the length that is valid.
                const int maxValidX = std::min(width - fillStartX, scaleX);
                // Cache local gradient vectors
                float left = internalNoiseMap(internalX);
                float right = internalNoiseMap(internalX + 1);
                // Cache points and attenuations TileMaps
                const auto& attenuationsX = kernelX.getAttenuations();
                // Loop over one interpolation kernel tile.
                for (int i = 0; i < maxValidX; ++i) {
                    // Interpolate based on the 4 surrounding internal noise points.
                    generatedNoiseMap(fillStartX + i) = interpolate1D(left, right, attenuationsX(i));
                }
            }

            template <int scaleX, int scaleY, int internalWidth, int internalLength, int width, int length>
            constexpr void fillSquare(int internalX, int internalY, int fillStartX, int fillStartY,
                const StealthTileMap::TileMapF<internalWidth, internalLength>& internalNoiseMap,
                StealthTileMap::TileMapF<width, length>& generatedNoiseMap, const InterpolationKernel<scaleX>& kernelX,
                const InterpolationKernel<scaleY>& kernelY) {
                // Only fill the part of the tile that is valid.
                const int maxValidX = std::min(width - fillStartX, scaleX);
                const int maxValidY = std::min(length - fillStartY, scaleY);
                // Cache local gradient vectors
                float topLeft = internalNoiseMap(internalX, internalY);
                float topRight = internalNoiseMap(internalX + 1, internalY);
                float bottomLeft = internalNoiseMap(internalX, internalY + 1);
                float bottomRight = internalNoiseMap(internalX + 1, internalY + 1);
                // Cache points and attenuations TileMaps
                const auto& attenuationsX = kernelX.getAttenuations();
                const auto& attenuationsY = kernelY.getAttenuations();
                // Loop over one interpolation kernel tile.
                for (int j = 0; j < maxValidY; ++j) {
                    for (int i = 0; i < maxValidX; ++i) {
                        // Interpolate based on the 4 surrounding internal noise points.
                        generatedNoiseMap(fillStartX + i, fillStartY + j) = interpolate2D(topLeft,
                            topRight, bottomLeft, bottomRight, attenuationsX(i), attenuationsY(j));
                    }
                }
            }
    };
} /* StealthWorldGenerator */

#endif
