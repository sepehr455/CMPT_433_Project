#include "include/GameServer.h"
#include "include/GameState.h"
#include "include/GameRender.h"
#include <chrono>
#include <thread>
#include <iostream>

int main() {
    // Start the TCP server
    GameServer server(8080);
    server.start();

    GameState gameState;
    GameRender renderer;

    // Track movement and rotation separately
    Direction lastDir = Direction::RIGHT;

    // Launch rendering in separate thread
    std::thread renderThread([&]() {
        renderer.run(gameState, lastDir);
    });

    // Main game loop (~60 FPS)
    while (true) {
        auto frameStart = std::chrono::steady_clock::now();

        Direction currentDir = server.getCurrentDirection();
        if (currentDir != Direction::NONE) {
            lastDir = currentDir;
            gameState.updateTankPosition(currentDir);
        }

        gameState.updateTurretRotation(server.getTurretRotationDelta());

        if (server.getButtonPressed()) {
            gameState.fireProjectile();
        }

        gameState.updateProjectiles();

        auto frameEnd = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - frameStart);
        if (elapsed.count() < 16) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16 - elapsed.count()));
        }
    }

    renderThread.join();
    return 0;
}