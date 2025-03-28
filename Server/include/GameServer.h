#pragma once
#include "Direction.h"
#include <string>
#include <netinet/in.h>
#include <atomic>

class GameServer {
public:
    GameServer(int port);
    ~GameServer();

    void start();
    Direction getCurrentDirection() const;
    int getTurretRotationDelta();

private:
    void receiveInput();
    void processInputToken(const std::string& token);

    int server_fd;
    int client_fd;
    sockaddr_in server_addr;
    sockaddr_in client_addr;
    socklen_t addr_len;

    // Made these atomic since they're accessed from different threads
    std::atomic<Direction> currentDirection;
    std::atomic<int> turretRotationDelta;
};