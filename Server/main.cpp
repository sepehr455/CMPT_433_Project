#include "include/GameServer.h"
#include "include/GameState.h"
#include "include/GameRender.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <mutex>

int main() {
    // Start the TCP server.
    GameServer server(8080);
    server.start();

    GameState gameState;
    GameRender renderer;

    // Track movement and rotation separately.
    Direction lastDir = Direction::RIGHT;

    // Launch rendering in a separate thread.
    std::thread renderThread([&]() {
        renderer.run(gameState, lastDir);
    });

    // Main game loop (~60 FPS).
    while (true) {
        auto frameStart = std::chrono::steady_clock::now();

        {
            // Lock the game state while updating.
            std::lock_guard<std::recursive_mutex> lock(gameState.getMutex());

            // Get all inputs at once.
            Direction currentDir = server.getCurrentDirection();
            int rotationDelta = server.getTurretRotationDelta();
            bool buttonPressed = server.getButtonPressed();

            // Update tank if direction changed.
            if (currentDir != Direction::NONE) {
                lastDir = currentDir;
                gameState.updateTankPosition(currentDir);
            }

            // Update turret if rotation changed.
            if (rotationDelta != 0) {
                gameState.updateTurretRotation(rotationDelta);
            }

            // Fire if button pressed.
            if (buttonPressed) {
                gameState.fireProjectile();
            }

            // Handle enemy shooting
            for (const auto& enemy : gameState.getEnemies()) {
                if (enemy->canShoot() && enemy->isActive()) {
                    float angle;
                    switch (enemy->getDirection()) {
                        case Direction::UP: angle = 0.0f; break;
                        case Direction::RIGHT: angle = 90.0f; break;
                        case Direction::DOWN: angle = 180.0f; break;
                        case Direction::LEFT: angle = 270.0f; break;
                        default: angle = 0.0f; break;
                    }
                    gameState.enemyFireProjectile(
                        enemy->getPosition().x,
                        enemy->getPosition().y,
                        angle
                    );
                    enemy->resetShootTimer();
                }
            }

            gameState.updateProjectiles();
        }

        auto frameEnd = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - frameStart);
        if (elapsed.count() < 16) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16 - elapsed.count()));
        }
    }

    renderThread.join();
    return 0;
}
