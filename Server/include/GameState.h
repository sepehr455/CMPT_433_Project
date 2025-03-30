#pragma once
#include "Direction.h"
#include "Tank.h"
#include "Enemy.h"
#include <vector>
#include <cmath>
#include <memory>
#include <random>
#include <mutex>  // Added for thread safety

struct Projectile {
    float x, y;
    float angle;
    float speed;
};

class GameState {
public:
    GameState();
    void updateTankPosition(Direction dir);
    void updateTurretRotation(int delta);
    void fireProjectile();
    void updateProjectiles();
    void spawnEnemy();
    void checkProjectileCollisions();

    const Tank& getTank() const;
    float getTurretAngle() const;
    const std::vector<Projectile>& getProjectiles() const;
    const std::vector<std::unique_ptr<Enemy>>& getEnemies() const;

    // Provide access to the mutex for external locking.
    std::recursive_mutex& getMutex() { return mtx; }

private:
    Tank tank;
    float turretAngle;
    std::vector<Projectile> projectiles;
    std::vector<std::unique_ptr<Enemy>> enemies;
    std::mt19937 rng;
    std::uniform_real_distribution<float> xDist;
    std::uniform_real_distribution<float> yDist;
    mutable std::recursive_mutex mtx;  // New mutex member
};
