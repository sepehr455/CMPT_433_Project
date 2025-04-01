#include "../include/Enemy.h"
#include <iostream>
#include <random>

Enemy::Enemy(float x, float y)
        : position(x, y), spawnTimer(0.0f), active(false), spawning(true),
          shootTimer(0.0f) {
    // Random direction
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dirDist(1, 4);
    direction = static_cast<Direction>(dirDist(gen));

    if (!texture.loadFromFile("Assets/enemy.png")) {
        std::cerr << "Failed to load enemy.png" << std::endl;
    }

    sprite.setTexture(texture);
    sprite.setOrigin(texture.getSize().x / 2.0f, texture.getSize().y / 2.0f);
    sprite.setScale(0.15f, 0.15f);
    sprite.setPosition(position);

    spawnIndicator.setRadius(RADIUS);
    spawnIndicator.setFillColor(sf::Color(255, 0, 0, 150));
    spawnIndicator.setOrigin(RADIUS, RADIUS);
    spawnIndicator.setPosition(position);
}

void Enemy::update(float dt) {
    if (spawning) {
        spawnTimer += dt;
        if (spawnTimer >= SPAWN_TIME) {
            spawning = false;
            active = true;
        }
    } else if (active) {
        shootTimer += dt;
    }
}

void Enemy::draw(sf::RenderWindow& window) const {
    if (spawning) {
        window.draw(spawnIndicator);
    } else if (active) {
        // Draw enemy with correct rotation based on direction
        sf::Sprite tempSprite = sprite;
        switch (direction) {
            case Direction::UP: tempSprite.setRotation(0); break;
            case Direction::RIGHT: tempSprite.setRotation(90); break;
            case Direction::DOWN: tempSprite.setRotation(180); break;
            case Direction::LEFT: tempSprite.setRotation(270); break;
            default: break;
        }
        window.draw(tempSprite);
    }
}

bool Enemy::isActive() const {
    return active;
}

bool Enemy::isSpawning() const {
    return spawning;
}

void Enemy::hit() {
    active = false;
    spawning = false;
}

sf::Vector2f Enemy::getPosition() const {
    return position;
}

float Enemy::getRadius() const {
    return RADIUS;
}

Direction Enemy::getDirection() const {
    return direction;
}

bool Enemy::canShoot() const {
    return active && shootTimer >= SHOOT_COOLDOWN;
}

void Enemy::resetShootTimer() {
    shootTimer = 0.0f;
}