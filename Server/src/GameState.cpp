#include "../include/GameState.h"
#include <algorithm>  // For std::max and std::min

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

const Tank& GameState::getTank() const {
    return tank;
}
