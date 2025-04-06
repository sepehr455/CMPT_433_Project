#include "../include/GameState.h"
#include "GameServer.h"
#include <algorithm>
#include <cmath>
#include <ctime>
#include <mutex>

GameState::GameState()
        : tank{512, 384, 10, 3},
          turretAngle(90.0f),
          rng(std::time(nullptr)),
          xDist(100.0f, 924.0f),
          yDist(100.0f, 668.0f),
          enemyShootTimer(0.0f),
          tankHitEffectTimer(0.0f),
          playerAlive(true),
          currentWave(0),
          enemiesKilledThisWave(0) {
    spawnEnemies();
}

void GameState::updateTankPosition(Direction dir) {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    if (!playerAlive) return;

    switch (dir) {
        case Direction::UP:
            tank.y -= tank.speed;
            break;
        case Direction::DOWN:
            tank.y += tank.speed;
            break;
        case Direction::LEFT:
            tank.x -= tank.speed;
            break;
        case Direction::RIGHT:
            tank.x += tank.speed;
            break;
        default:
            break;
    }

    // Keep tank within screen bounds
    tank.x = std::max(0, std::min(tank.x, 1024));
    tank.y = std::max(0, std::min(tank.y, 768));
}

// Rotate turret by a delta (positive or negative)
void GameState::updateTurretRotation(int delta) {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    if (!playerAlive) return;

    turretAngle += delta * 10.0f;
    turretAngle = fmod(turretAngle, 360.0f);
    if (turretAngle < 0) turretAngle += 360.0f;
}

// Create a new projectile from the tank
void GameState::fireProjectile() {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    if (!playerAlive) return;

    Projectile p;
    p.x = tank.x;
    p.y = tank.y;
    p.angle = turretAngle;
    p.speed = 15.0f;
    p.isEnemyProjectile = false;
    projectiles.push_back(p);
}

// Create a new projectile from an enemy
void GameState::enemyFireProjectile(float x, float y, float angle) {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    Projectile p;
    p.x = x;
    p.y = y;
    p.angle = angle;
    p.speed = 10.0f;
    p.isEnemyProjectile = true;
    projectiles.push_back(p);
}

// Move all projectiles and handle off-screen cleanup + collision
void GameState::updateProjectiles() {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    const float pi = 3.14159265f;

    // Decrease hit effect timer
    if (tankHitEffectTimer > 0) {
        tankHitEffectTimer -= 1.0f / 60.0f; // Assuming 60 FPS
    }

    for (auto& p : projectiles) {
        float radians = (p.angle - 90) * pi / 180.0f;
        p.x += p.speed * cos(radians);
        p.y += p.speed * sin(radians);
    }

    projectiles.erase(
            std::remove_if(projectiles.begin(), projectiles.end(),
                           [](const Projectile &p) {
                               return p.x < 0 || p.x > 1024 || p.y < 0 || p.y > 768;
                           }),
            projectiles.end());

    checkProjectileCollisions();
    checkTankHit();
}

// Spawn a new wave of enemies if all are cleared
void GameState::spawnEnemies() {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    // Only spawn new enemies if all current ones are dead
    bool allDead = std::all_of(enemies.begin(), enemies.end(),
                               [](const std::unique_ptr<Enemy>& e) {
                                   return !e->isActive() && !e->isSpawning();
                               });

    if (allDead || enemies.empty()) {
        // Calculate min/max enemies based on wave
        int baseEnemies = 1 + (currentWave / WAVES_FOR_ENEMY_INCREASE);
        int minEnemies = baseEnemies;
        int maxEnemies = baseEnemies + 4;

        std::uniform_int_distribution<int> countDist(minEnemies, maxEnemies);
        int enemyCount = countDist(rng);

        // Spawn new enemies at random positions
        for (int i = 0; i < enemyCount; i++) {
            float x = xDist(rng);
            float y = yDist(rng);
            enemies.push_back(std::make_unique<Enemy>(x, y));
        }

        enemiesKilledThisWave = 0;
    }
}

// Handle projectile collision with enemies
void GameState::checkProjectileCollisions() {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    for (auto it = projectiles.begin(); it != projectiles.end();) {
        bool hit = false;

        // Only player projectiles can hit enemies
        if (!it->isEnemyProjectile) {
            for (auto &enemy: enemies) {
                if (enemy->isActive()) {
                    float dx = it->x - enemy->getPosition().x;
                    float dy = it->y - enemy->getPosition().y;
                    float distance = std::sqrt(dx * dx + dy * dy);
                    if (distance < enemy->getRadius()) {
                        enemy->hit();
                        hit = true;
                        enemiesKilledThisWave++;
                        break;
                    }
                }
            }
        }

        // Remove projectile if it hit
        if (hit) {
            it = projectiles.erase(it);
        } else {
            ++it;
        }
    }

    // Remove dead enemies
    enemies.erase(
            std::remove_if(enemies.begin(), enemies.end(),
                           [](const std::unique_ptr<Enemy> &e) {
                               return !e->isActive() && !e->isSpawning();
                           }),
            enemies.end());

    // Check if we should spawn new enemies
    if (enemies.empty()) {
        currentWave++;
        spawnEnemies();
    }
}

// Check if tank was hit by enemy projectiles
void GameState::checkTankHit() {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    if (!playerAlive) return;

    for (auto it = projectiles.begin(); it != projectiles.end();) {

        // Player tank hit detection
        if (it->isEnemyProjectile) {
            float dx = it->x - tank.x;
            float dy = it->y - tank.y;
            float distance = std::sqrt(dx * dx + dy * dy);
            if (distance < 20.0f) {
                tank.health--;
                tankHitEffectTimer = HIT_EFFECT_DURATION;
                it = projectiles.erase(it);

                if (tank.health <= 0) {
                    playerAlive = false;
                }

                if (server) {
                    server->sendHitMessage();
                }

                continue;
            }
        }
        ++it;
    }
}

// Simple getters
const Tank &GameState::getTank() const { return tank; }

float GameState::getTurretAngle() const { return turretAngle; }

const std::vector<Projectile> &GameState::getProjectiles() const { return projectiles; }

const std::vector<std::unique_ptr<Enemy>> &GameState::getEnemies() const { return enemies; }

bool GameState::isPlayerAlive() const { return playerAlive; }

float GameState::getTankHitEffect() const { return tankHitEffectTimer; }

// Link to server to allow outbound messages (e.g., tank hit)
void GameState::setServer(GameServer *srv) {
    server = srv;
}
