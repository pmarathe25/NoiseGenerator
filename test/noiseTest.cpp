#include "Color/ColorPalette.hpp"
#include "NoiseGenerator/NoiseGenerator.hpp"
#include <stealthutil>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <functional>
#include <iostream>

using StealthColor::Color, StealthColor::applyPalette, StealthColor::GradientColorPalette;

constexpr int WINDOW_X = 800;
constexpr int WINDOW_Y = 800;
constexpr int NUM_LAYERS = 1;
constexpr int FRAMERATE = 0;

const GradientColorPalette noisePalette{Color(0, 0, 0), Color(255, 255, 255)};

template <typename TileMapType>
constexpr sf::Sprite spriteFromColorMap(const TileMapType& colors, sf::Texture& texture) {
    sf::Image im;
    sf::Sprite sprite;
    im.create(colors.width(), colors.length(), (uint8_t*) colors.data());
    texture.loadFromImage(im);
    sprite.setTexture(texture);
    return sprite;
}

int main() {
    // Window
    sf::RenderWindow window(sf::VideoMode(WINDOW_X, WINDOW_Y), "Noise Test");

    long long totalTime = 0;
    int numFrames = 0;

    // float smoothness = 0.0f;

    StealthTileMap::TileMapF<WINDOW_X, WINDOW_Y, NUM_LAYERS> noise{};
    while (window.isOpen()) {
        auto start = std::chrono::steady_clock::now();

        // StealthNoiseGenerator::generateOctaves<WINDOW_X, WINDOW_Y, NUM_LAYERS, 200, 200, 100, 1>(noise,
        //     std::normal_distribution{0.5f, 1 / 6.0f}, stealth::getCurrentTime());
        StealthNoiseGenerator::generateOctaves<WINDOW_X, WINDOW_Y, NUM_LAYERS, 200, 200, 100, 12>(noise,
            std::normal_distribution{0.5f, 1 / 6.0f}, stealth::getCurrentTime());
        // StealthNoiseGenerator::generateOctaves<WINDOW_X, WINDOW_Y, NUM_LAYERS, 200, 200, 100, 12>(noise,
        //     std::uniform_real_distribution{0.0f, 1.0f}, stealth::getCurrentTime());
        // StealthNoiseGenerator::generateOctaves<WINDOW_X, WINDOW_Y, 200, 200, 12>(noise,
        //     std::normal_distribution{0.5f, 1 / 6.0f}, stealth::getCurrentTime(), 0.5f);
        // StealthNoiseGenerator::generateOctaves<WINDOW_X, WINDOW_Y, 200, 200, 12>(noise,
        //     std::uniform_real_distribution{0.0f, 1.0f}, stealth::getCurrentTime(), 0.5f);
        // StealthNoiseGenerator::generateOctaves<WINDOW_X, 200, 8>(noise,
        //     std::normal_distribution{0.5f, 1 / 6.0f}, stealth::getCurrentTime(), 0.5f);

        auto end = std::chrono::steady_clock::now();
        totalTime += std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "Average Time:  " << (totalTime / ++numFrames) << " milliseconds" << '\r' << std::flush;

        auto colorMap = applyPalette(noisePalette, noise);
        // Display each layer of noise on-screen.
        sf::Texture noiseTexture;
        for (int i = 0; i < NUM_LAYERS; ++i) {
            sf::Sprite noiseSprite = spriteFromColorMap(StealthTileMap::layer(colorMap, i), noiseTexture);
            // Draw
            window.draw(noiseSprite);
            // Display.
            window.display();
            // Handle events.
            sf::Event event;
            while (window.pollEvent(event)) {
                if(event.type == sf::Event::Closed) {
                    window.close();
                }
            }
            if constexpr (FRAMERATE > 0) stealth::sleepMS((long) 1000.0f / FRAMERATE - std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());
        }
    }
    std::cout << std::endl;
}
