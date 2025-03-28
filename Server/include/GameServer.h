#pragma once
#include "Direction.h"
#include <string>
#include <netinet/in.h>

class GameServer {
public:
    GameServer(int port);
    ~GameServer();

    void start();
    Direction getCurrentDirection() const;
    int getTurretRotationDelta();

private:
    void receiveInput();

    int server_fd;
    int client_fd;
    sockaddr_in server_addr;
    sockaddr_in client_addr;
    socklen_t addr_len;

    Direction currentDirection;
    int turretRotationDelta;
};