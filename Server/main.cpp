#include "include/GameServer.h"
#include <iostream>
#include <chrono>
#include <thread>

int main() {
    GameServer server(8080);
    server.start();

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        Direction dir = server.getCurrentDirection();

        std::string directionStr;
        switch (dir) {
            case Direction::UP: directionStr = "UP"; break;
            case Direction::DOWN: directionStr = "DOWN"; break;
            case Direction::LEFT: directionStr = "LEFT"; break;
            case Direction::RIGHT: directionStr = "RIGHT"; break;
            default: directionStr = "NONE"; break;
        }

        std::cout << "Current Direction: " << directionStr << std::endl;
    }

    return 0;
}
