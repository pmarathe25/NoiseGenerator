#ifndef STEALTH_INTERPOLATION_H
#define STEALTH_INTERPOLATION_H
#include <Tensor3>
#include <random>

namespace Stealth::Noise {
    using DefaultGenerator = std::mt19937;
    using DefaultDistribution = std::uniform_real_distribution<float>;

    namespace {
        float attenuationPolynomial(float distance) noexcept {
            // Distance is a value between 0.0 and 1.0f.
            return (6 * pow(distance, 5) - 15 * pow(distance, 4) + 10 * pow(distance, 3));
        }

        template <int scale>
        Tensor::Tensor3F<scale> generateAttenuations() noexcept {
            Tensor::Tensor3F<scale> attenuations;
            for (int i = 0; i < scale; ++i) {
                attenuations(i) = attenuationPolynomial(i / (float) scale);
            }
            return attenuations;
        }

        template <int width, int length = 1, int height = 1, typename Distribution, typename Generator = DefaultGenerator>
        auto generateInternalNoiseMap(long desiredSeed, Distribution&& distribution
            = DefaultDistribution{0.f, 1.f}, Generator&& generator = DefaultGenerator{}) {
            generator.seed(desiredSeed);
            constexpr int size = width * length * height;
            Tensor::Tensor3F<size> tmp;
            for (int i = 0; i < size; ++i) {
                tmp(i) = distribution(generator);
            }
            return tmp;
        }

        constexpr int32_t ceilDivide(int32_t x, int32_t y)
        {
            return (x + y - 1) / y;
        }
    } /* Anonymous namespace */
} /* StealthWorldGenerator */

#endif
