#pragma once

#include <SFML/Graphics.hpp>
#include "GameState.h"
#include "Direction.h"
#include <atomic>

// Responsible for rendering the game window and visual elements
class GameRender {
public:
    GameRender();
    ~GameRender();

    // Main rendering loop - draws everything to the screen
    void run(GameState& state, Direction& currentDir, std::atomic<bool>& running);

private:
    // Window and background rendering
    sf::RenderWindow window;
    sf::Texture backgroundTexture;
    sf::Sprite backgroundSprite;

    // Tank rendering
    sf::Texture bodyTexture;
    sf::Texture turretTexture;
    sf::Sprite bodySprite;
    sf::Sprite turretSprite;

    // Projectile visuals
    sf::CircleShape projectileShape;

    // Time tracking for animations and updates
    sf::Clock deltaClock;

    // Visual effect for when the tank gets hit
    sf::RectangleShape hitEffect;

    // Game Over UI
    sf::Font gameOverFont;
    sf::Text gameOverText;
    sf::RectangleShape overlay;

    // Wave number UI
    sf::Text waveText;
    sf::Font waveFont;
};