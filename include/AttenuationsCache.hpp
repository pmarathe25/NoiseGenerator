#ifndef STEALTH_INTERPOLATION_H
#define STEALTH_INTERPOLATION_H
#include "TileMap/TileMap.hpp"
#include <stealthutil>

namespace StealthNoiseGenerator {
    namespace {
        template <int scale>
        constexpr StealthTileMap::TileMapF<scale> generateAttenuations() noexcept {
            StealthTileMap::TileMapF<scale> attenuations;
            for (int i = scale - 1; i >= 0; --i) {
                attenuations(i) = attenuationPolynomial((i + 0.5f) / scale);
            }
            return attenuations;
        }

        // Attenuations cache
        template <int scale>
        static const StealthTileMap::TileMapF<scale> AttenuationsCache = generateAttenuations<scale>();
    }
} /* StealthWorldGenerator */

#endif
