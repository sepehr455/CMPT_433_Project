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

    // Movement and rotation
    void updateTankPosition(Direction dir);
    void updateTurretRotation(int delta);

    // Firing methods
    void fireProjectile();
    void enemyFireProjectile(float x, float y, float angle);

    // Updates called every frame
    void updateProjectiles();
    void spawnEnemies();
    void checkProjectileCollisions();
    void checkTankHit();

    // Getters
    const Tank &getTank() const;
    float getTurretAngle() const;
    const std::vector<Projectile>& getProjectiles() const;
    const std::vector<std::unique_ptr<Enemy>>& getEnemies() const;
    bool isPlayerAlive() const;

    float getTankHitEffect() const;
    int getCurrentWave() const { return currentWave; }

    // Expose internal mutex for thread safety
    std::recursive_mutex& getMutex() { return mtx; }

    // Link to server for feedback (e.g., hit messages)
    void setServer(GameServer* srv);

    void restoreTankHealth();


private:
    Tank tank;
    float turretAngle;

    std::vector<Projectile> projectiles;
    std::vector<std::unique_ptr<Enemy>> enemies;

    // Enemy number and location
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