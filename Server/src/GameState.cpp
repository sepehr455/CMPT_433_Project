#include "../include/GameState.h"
#include <algorithm>
#include <cmath>

GameState::GameState() {
    tank.x = 512;
    tank.y = 384;
    tank.speed = 10;
    turretAngle = 90.0f;
}

void GameState::updateTankPosition(Direction dir) {
    switch (dir) {
        case Direction::UP:    tank.y -= tank.speed; break;
        case Direction::DOWN:  tank.y += tank.speed; break;
        case Direction::LEFT:  tank.x -= tank.speed; break;
        case Direction::RIGHT: tank.x += tank.speed; break;
        default: break;
    }
    tank.x = std::max(0, std::min(tank.x, 1024));
    tank.y = std::max(0, std::min(tank.y, 768));
}

void GameState::updateTurretRotation(int delta) {
    turretAngle += delta * 2.0f;
    turretAngle = fmod(turretAngle, 360.0f);
    if (turretAngle < 0) turretAngle += 360.0f;
}


void GameState::fireProjectile() {
    Projectile p;
    p.x = tank.x;
    p.y = tank.y;
    p.angle = turretAngle;
    p.speed = 15.0f;  // Projectile speed
    projectiles.push_back(p);
}

void GameState::updateProjectiles() {
    const float pi = 3.14159265f;

    for (auto& p : projectiles) {
        // Convert to radians and adjust for SFML's coordinate system
        // (0° = up, 90° = right in SFML, while our turret uses 0° = right)
        float radians = (p.angle - 90) * pi / 180.0f;  // Subtract 90 degrees
        p.x += p.speed * cos(radians);
        p.y += p.speed * sin(radians);
    }

    projectiles.erase(
            std::remove_if(projectiles.begin(), projectiles.end(),
                           [](const Projectile& p) {
                               return p.x < 0 || p.x > 1024 || p.y < 0 || p.y > 768;
                           }),
            projectiles.end());
}

const Tank& GameState::getTank() const {
    return tank;
}

float GameState::getTurretAngle() const {
    return turretAngle;
}

const std::vector<Projectile>& GameState::getProjectiles() const {
    return projectiles;
}