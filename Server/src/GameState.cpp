#include "../include/GameState.h"
#include <algorithm>
#include <cmath>

GameState::GameState() {
    tank.x = 512;
    tank.y = 384;
    tank.speed = 10;
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

const Tank& GameState::getTank() const {
    return tank;
}

float GameState::getTurretAngle() const {
    return turretAngle;
}