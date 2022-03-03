#include "interfaces/NoiseGenerator"
#include <Color>
#include <chrono>
#include <thread>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <functional>
#include <iostream>

using StealthColor::Color, StealthColor::applyPalette, StealthColor::GradientColorPalette;

constexpr int WINDOW_X = 500;
constexpr int WINDOW_Y = 500;
constexpr int NUM_LAYERS = 96;
constexpr int FRAMERATE = 24;

const GradientColorPalette noisePalette{Color(0, 0, 0), Color(255, 255, 255)};

template <typename TileMapType>
sf::Sprite spriteFromColorMap(const TileMapType& colors, sf::Texture& texture) {
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
    long seed = 0;

    while (window.isOpen()) {
        auto start = std::chrono::steady_clock::now();

        Stealth::Tensor::Tensor3F<WINDOW_X, WINDOW_Y, NUM_LAYERS> noise{};
        Stealth::Noise::generateOctaves<WINDOW_X, WINDOW_Y, NUM_LAYERS, WINDOW_X, WINDOW_Y, NUM_LAYERS, 8>(noise,
            std::normal_distribution{0.5f, 0.3f}, seed++);

        auto end = std::chrono::steady_clock::now();
        totalTime += std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "Average Time:  " << (totalTime / ++numFrames) << " milliseconds" << '\r' << std::flush;

        auto colorMap = applyPalette(noisePalette, noise);
        // Display each layer of noise on-screen.
        sf::Texture noiseTexture;
        for (int i = 0; i < NUM_LAYERS; ++i) {
            sf::Sprite noiseSprite = spriteFromColorMap(Stealth::Tensor::layer(colorMap, i), noiseTexture);
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
            if constexpr (FRAMERATE > 0)
            {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(
                        (long) 1000.0f / FRAMERATE
                    )
                );

            } 
        }
    }
    std::cout << std::endl;
}
