#pragma once
#include "Direction.h"
#include "Tank.h"
#include <vector>
#include <cmath>

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

    const Tank& getTank() const;
    float getTurretAngle() const;
    const std::vector<Projectile>& getProjectiles() const;

private:
    Tank tank;
    float turretAngle;
    std::vector<Projectile> projectiles;
};