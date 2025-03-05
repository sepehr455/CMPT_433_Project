#ifndef GAMESERVER_H
#define GAMESERVER_H

#include <iostream>
#include <string>
#include <netinet/in.h>
#include <unistd.h>
#include "Direction.h"


class GameServer {
private:
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len;
    Direction currentDirection;

public:
    GameServer(int port);
    ~GameServer();

    void start();
    void receiveInput();
    [[nodiscard]] Direction getCurrentDirection() const;
};

#endif // GAMESERVER_H
