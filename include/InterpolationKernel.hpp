#ifndef STEALTH_INTERPOLATION_H
#define STEALTH_INTERPOLATION_H
#include "TileMap/TileMap.hpp"
#include <stealthutil>

namespace StealthNoiseGenerator {
    class InterpolationKernelBase { };

    // Maintains a cache of points and attenuations to use for each possible location of a pixel.
    template <int scale = 1>
    class InterpolationKernel : public InterpolationKernelBase {
        public:
            constexpr InterpolationKernel() {
                initializeKernel();
            }

            constexpr InterpolationKernel(const InterpolationKernel& other) noexcept = default;

            constexpr InterpolationKernel(InterpolationKernel&& other) noexcept = default;

            constexpr const auto& getPoints() const noexcept {
                return points;
            }

            constexpr const auto& getAttenuations() const noexcept {
                return attenuations;
            }

        private:
            StealthTileMap::TileMap<float, scale> points{};
            StealthTileMap::TileMap<float, scale> attenuations{};

            constexpr void initializeKernel() noexcept {
                // Optimally initialize kernel. Only need to compute 1/8th of the kernel.
                constexpr int halfBound = ceilDivide(scale, 2);
                initializeHalf(halfBound);
                reflectHalf(halfBound);
            }

            constexpr float calculatePoint(int index) const noexcept {
                // Compute a relative location.
                return (index / (float) scale) + (0.5f / scale);
            }

            constexpr void initializeHalf(int halfBound) {
                for (int i = 0; i < halfBound; ++i) {
                    float point = calculatePoint(i);
                    points(i) = point;
                    attenuations(i) = attenuationPolynomial(point);
                }
            }

            constexpr void reflectHalf(int halfBound) {
                for (int i = scale - 1, index = 0; i >= halfBound; --i, ++index) {
                    float point = 1.0f - points(index);
                    points(i) = point;
                    attenuations(i) = attenuationPolynomial(point);
                }
            }
    };

} /* StealthWorldGenerator */

#endif
