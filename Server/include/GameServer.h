#pragma once

#include "Direction.h"
#include <string>
#include <netinet/in.h>
#include <atomic>

// Forward declaration to avoid circular include
class GameState;

class GameServer {
public:
    explicit GameServer(int port);
    ~GameServer();

    // Start and stop functions for server
    void start();
    void stop();

    [[nodiscard]] Direction getCurrentDirection() const;
    int getTurretRotationDelta();
    bool getButtonPressed();

    void sendTankHealth(int health) const;
    void sendGameOver(const char* message) const;
    void sendHitMessage() const;

    // Link to the GameState to allow invoking actions like cheats
    void setGameState(GameState* gs);

private:
    void receiveInput();
    void processInputToken(const std::string& token);
    void registerServerCleanup();

    int server_fd;
    int client_fd;
    sockaddr_in server_addr;
    sockaddr_in client_addr;
    socklen_t addr_len;

    std::atomic<Direction> currentDirection;
    std::atomic<int> turretRotationDelta;
    std::atomic<bool> buttonPressed;
    GameState* gameState = nullptr;
};
