#pragma once

#include "Direction.h"
#include "Tank.h"
#include "Enemy.h"
#include <vector>
#include <cmath>
#include <memory>
#include <random>
#include <mutex>

class GameServer;

struct Projectile {
    float x, y;
    float angle;
    float speed;
    bool isEnemyProjectile;
};

class GameState {
public:
    GameState();

    void updateTankPosition(Direction dir);

    void updateTurretRotation(int delta);

    void fireProjectile();

    void enemyFireProjectile(float x, float y, float angle);

    void updateProjectiles();

    void spawnEnemies();

    void checkProjectileCollisions();

    void checkTankHit();

    const Tank &getTank() const;

    float getTurretAngle() const;

    const std::vector<Projectile> &getProjectiles() const;

    const std::vector<std::unique_ptr<Enemy>> &getEnemies() const;

    bool isPlayerAlive() const;

    float getTankHitEffect() const;

    std::recursive_mutex &getMutex() { return mtx; }

    void setServer(GameServer *srv);

    int getCurrentWave() const { return currentWave; }

private:
    Tank tank;
    float turretAngle;
    std::vector<Projectile> projectiles;
    std::vector<std::unique_ptr<Enemy>> enemies;
    std::mt19937 rng;
    std::uniform_real_distribution<float> xDist;
    std::uniform_real_distribution<float> yDist;
    mutable std::recursive_mutex mtx;
    float enemyShootTimer;
    float tankHitEffectTimer;
    bool playerAlive;
    static constexpr float ENEMY_SHOOT_INTERVAL = 3.0f;
    static constexpr float HIT_EFFECT_DURATION = 0.5f;


    int currentWave;
    int enemiesKilledThisWave;
    static constexpr int WAVES_FOR_ENEMY_INCREASE = 5;

    GameServer *server = nullptr;
};