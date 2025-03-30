#include "../include/GameState.h"
#include <algorithm>
#include <cmath>
#include <ctime>
#include <mutex>

GameState::GameState()
        : tank{512, 384, 10, 100},
          turretAngle(90.0f),
          rng(std::time(nullptr)),
          xDist(100.0f, 924.0f),
          yDist(100.0f, 668.0f) {
    spawnEnemy();
}

void GameState::updateTankPosition(Direction dir) {
    std::lock_guard<std::recursive_mutex> lock(mtx);
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
    std::lock_guard<std::recursive_mutex> lock(mtx);
    turretAngle += delta * 10.0f;
    turretAngle = fmod(turretAngle, 360.0f);
    if (turretAngle < 0) turretAngle += 360.0f;
}

void GameState::fireProjectile() {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    Projectile p;
    p.x = tank.x;
    p.y = tank.y;
    p.angle = turretAngle;
    p.speed = 15.0f;
    projectiles.push_back(p);
}

void GameState::updateProjectiles() {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    const float pi = 3.14159265f;

    for (auto& p : projectiles) {
        float radians = (p.angle - 90) * pi / 180.0f;
        p.x += p.speed * cos(radians);
        p.y += p.speed * sin(radians);
    }

    projectiles.erase(
            std::remove_if(projectiles.begin(), projectiles.end(),
                           [](const Projectile& p) {
                               return p.x < 0 || p.x > 1024 || p.y < 0 || p.y > 768;
                           }),
            projectiles.end());

    checkProjectileCollisions();
}

void GameState::spawnEnemy() {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    float x = xDist(rng);
    float y = yDist(rng);
    enemies.push_back(std::make_unique<Enemy>(x, y));
}

void GameState::checkProjectileCollisions() {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    for (auto it = projectiles.begin(); it != projectiles.end(); ) {
        bool hit = false;
        for (auto& enemy : enemies) {
            if (enemy->isActive()) {
                float dx = it->x - enemy->getPosition().x;
                float dy = it->y - enemy->getPosition().y;
                float distance = std::sqrt(dx * dx + dy * dy);
                if (distance < enemy->getRadius()) {
                    enemy->hit();
                    hit = true;
                    spawnEnemy();
                    break;
                }
            }
        }
        if (hit) {
            it = projectiles.erase(it);
        } else {
            ++it;
        }
    }

    // Remove dead enemies
    enemies.erase(
            std::remove_if(enemies.begin(), enemies.end(),
                           [](const std::unique_ptr<Enemy>& e) {
                               return !e->isActive() && !e->isSpawning();
                           }),
            enemies.end());
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

const std::vector<std::unique_ptr<Enemy>>& GameState::getEnemies() const {
    return enemies;
}
