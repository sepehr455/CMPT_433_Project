#pragma once
#include "Tank.h"
#include "Direction.h"

class GameState {
private:
    Tank tank;
public:
    GameState();
    void updateTankPosition(Direction dir);
    const Tank& getTank() const;
};
