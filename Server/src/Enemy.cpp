#include "../include/Enemy.h"
#include <iostream>

Enemy::Enemy(float x, float y)
        : position(x, y), spawnTimer(0.0f), active(false), spawning(true) {

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
    }
}

void Enemy::draw(sf::RenderWindow& window) const {
    if (spawning) {
        window.draw(spawnIndicator);
    } else if (active) {
        window.draw(sprite);
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