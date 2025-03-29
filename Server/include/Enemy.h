#pragma once
#include <SFML/Graphics.hpp>

class Enemy {
public:
    Enemy(float x, float y);
    void update(float dt);
    void draw(sf::RenderWindow& window) const;
    bool isActive() const;
    bool isSpawning() const;
    void hit();
    sf::Vector2f getPosition() const;
    float getRadius() const;

private:
    sf::Vector2f position;
    sf::Texture texture;
    sf::Sprite sprite;
    sf::CircleShape spawnIndicator;
    float spawnTimer;
    bool active;
    bool spawning;
    static constexpr float SPAWN_TIME = 3.0f;
    static constexpr float RADIUS = 20.0f;
};