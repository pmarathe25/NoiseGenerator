#include "Color/ColorPalette.hpp"
#include "NoiseGenerator/NoiseGenerator.hpp"
#include <stealthutil>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <functional>

using StealthColor::Color, StealthColor::applyPalette, StealthColor::GradientColorPalette;

constexpr int WINDOW_X = 800;
constexpr int WINDOW_Y = 800;
constexpr int NUM_LAYERS = 120;
constexpr int FRAMERATE = 60;

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

constexpr float doubleUp(float in) {
    return in * 2.0;
}

constexpr float threshold(float in, float threshold) {
    return (in > threshold) ? in : 0.0f;
}

int main() {
    // Window
    sf::RenderWindow window(sf::VideoMode(WINDOW_X, WINDOW_Y), "Noise Test");

    long long totalTime = 0;
    int numFrames = 0;

    float smoothness = 0.0f;

    while (window.isOpen()) {
        auto start = std::chrono::steady_clock::now();

        smoothness += (smoothness > 1.0f) ? -1.0f : 0.1;
        auto noise = StealthNoiseGenerator::generateOctaves<WINDOW_X, WINDOW_Y, NUM_LAYERS, 400, 400, 200>(smoothness);

        // noise = (noise < noise2) + (noise > noise2); // Should be all 1s (white)
        // noise = noise && (noise < noise2);
        // noise = noise * (noise < noise2);
        // noise = noise2;
        // noise = -1.0f * noise2 + 1.0f;
        // noise = noise * (noise < (noise2 * 2));
        // noise = noise - noise2;
        // StealthWorldGenerator::TileMapF<WINDOW_Y, WINDOW_X> noiseTest = StealthWorldGenerator::apply(doubleUp, noise);
        // noiseTest = StealthWorldGenerator::apply(std::bind(threshold, std::placeholders::_1, 0.25f), noise);

        auto end = std::chrono::steady_clock::now();
        totalTime += std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "Average Time:  " << (totalTime / ++numFrames) << " milliseconds" << '\r';

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
            sleepMS((long) 1000.0f / FRAMERATE);
        }
    }
}
