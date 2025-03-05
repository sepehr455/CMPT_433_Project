#include "include/GameServer.h"
#include "include/GameState.h"
#include "include/GameRender.h"
#include <chrono>
#include <thread>

int main() {
    // Start the TCP server to receive joystick directions.
    GameServer server(8080);
    server.start();

    GameState gameState;
    GameRender renderer;

    // Use lastDir to hold the last non-NONE direction; default to RIGHT.
    // We'll use lastDir for rendering orientation.
    Direction currentDir = Direction::NONE;
    Direction lastDir = Direction::RIGHT;

    // Launch the rendering loop in a separate thread.
    std::thread renderThread([&]() {
        renderer.run(gameState, lastDir);
    });

    // Main loop: Update game state only when a new non-NONE direction is received.
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS update rate.

        // Retrieve new direction from the server.
        currentDir = server.getCurrentDirection();

        // If a new non-NONE direction is received, update both lastDir and the game state.
        if (currentDir != Direction::NONE) {
            lastDir = currentDir;
            gameState.updateTankPosition(currentDir);
        }
        // If currentDir is NONE, do nothing so the tank stops moving.
    }

    renderThread.join();
    return 0;
}
