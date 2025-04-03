#include "include/GameServer.h"
#include "include/GameState.h"
#include "include/GameRender.h"
#include "include/Shutdown.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <mutex>
#include <csignal>
#include <atomic>

// Signal handler for graceful shutdown
void signalHandler(int) {
    ShutdownModule::requestShutdown();
}

int main() {
    // Register signal handlers
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // Start the TCP server
    GameServer server(8080);
    server.start();

    GameState gameState;
    gameState.setServer(&server);
    GameRender renderer;

    // Track movement and rotation separately.
    Direction lastDir = Direction::RIGHT;

    // Atomic flag for render thread
    std::atomic<bool> renderThreadRunning{true};

    // Launch rendering in a separate thread.
    std::thread renderThread([&]() {
        renderer.run(gameState, lastDir, renderThreadRunning);
    });

    // Main game loop (~60 FPS)
    bool gameOverNotified = false;
    auto gameOverStart = std::chrono::steady_clock::time_point();

    while (!ShutdownModule::isShutdownRequested()) {
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
            for (const auto &enemy: gameState.getEnemies()) {
                if (enemy->canShoot() && enemy->isActive()) {
                    float angle;
                    switch (enemy->getDirection()) {
                        case Direction::UP:
                            angle = 0.0f;
                            break;
                        case Direction::RIGHT:
                            angle = 90.0f;
                            break;
                        case Direction::DOWN:
                            angle = 180.0f;
                            break;
                        case Direction::LEFT:
                            angle = 270.0f;
                            break;
                        default:
                            angle = 0.0f;
                            break;
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

            // Game Over Logic
            if (!gameState.isPlayerAlive() && !gameOverNotified) {
                gameOverNotified = true;
                server.sendGameOver("GAME_OVER\n");
                gameOverStart = std::chrono::steady_clock::now();
            } else if (gameState.isPlayerAlive()) {
                // Only send health updates if player is alive
                server.sendTankHealth(gameState.getTank().health);
            }
        }

        // If we have notified game over, wait 5 seconds, then shut down.
        if (gameOverNotified) {
            auto now = std::chrono::steady_clock::now();
            auto secs = std::chrono::duration_cast<std::chrono::seconds>(now - gameOverStart).count();
            if (secs >= 5) {
                ShutdownModule::requestShutdown();
            }
        }

        auto frameEnd = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - frameStart);
        if (elapsed.count() < 16) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16 - elapsed.count()));
        }
    }

    // Cleanup
    renderThreadRunning = false;
    try {
        if (renderThread.joinable()) {
            renderThread.join();
        }
    } catch (const std::exception &e) {
        std::cerr << "Error joining render thread: " << e.what() << std::endl;
    }

    std::cout << "Server shutdown complete" << std::endl;
    return 0;
}
