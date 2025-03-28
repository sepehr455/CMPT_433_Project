#pragma once
#include "Tank.h"
#include "Direction.h"

class GameState {
public:
    GameState();
    void updateTankPosition(Direction dir);
    void updateTurretRotation(int delta);
    const Tank& getTank() const;
    float getTurretAngle() const;

private:
    Tank tank;
    float turretAngle = 90.0f;
};