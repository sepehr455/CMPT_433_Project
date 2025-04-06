#pragma once
#include <SFML/Graphics.hpp>
#include "Direction.h"

class Enemy {
public:
    // Constructor - initializes enemy at a specific position
    Enemy(float x, float y);

    // Called every frame to update timers and internal state
    void update(float dt);
    void draw(sf::RenderWindow& window) const;

    // State checks
    bool isActive() const;
    bool isSpawning() const;

    // Called when the enemy is hit by a projectile
    void hit();

    // Basic getters for gameplay logic (e.g., collisions, AI)
    sf::Vector2f getPosition() const;
    static float getRadius() ;
    Direction getDirection() const;

    // Shooting behavior
    bool canShoot() const;
    void resetShootTimer();

private:
    // Visual representation
    sf::Vector2f position;
    sf::Texture texture;
    sf::Sprite sprite;
    sf::CircleShape spawnIndicator;

    // Internal state
    float spawnTimer;
    bool active;
    bool spawning;
    Direction direction;
    float shootTimer;
    static constexpr float SPAWN_TIME = 3.0f;
    static constexpr float RADIUS = 20.0f;
    static constexpr float SHOOT_COOLDOWN = 3.0f;
};