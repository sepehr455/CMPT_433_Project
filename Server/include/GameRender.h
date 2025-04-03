#pragma once
#include <SFML/Graphics.hpp>
#include "GameState.h"
#include "Direction.h"
#include <atomic>

class GameRender {
public:
    GameRender();
    ~GameRender();

    void run(GameState& state, Direction& currentDir, std::atomic<bool>& running);

private:
    sf::RenderWindow window;
    sf::Texture backgroundTexture;
    sf::Sprite backgroundSprite;

    sf::Texture bodyTexture;
    sf::Texture turretTexture;
    sf::Sprite bodySprite;
    sf::Sprite turretSprite;

    sf::CircleShape projectileShape;
    sf::Clock deltaClock;
    sf::RectangleShape hitEffect;

    // For Game Over
    sf::Font gameOverFont;
    sf::Text gameOverText;
    sf::RectangleShape overlay;
};