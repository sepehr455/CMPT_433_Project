#include "../include/GameServer.h"
#include <cstring>
#include <arpa/inet.h>
#include <thread>
#include <iostream>
#include <unistd.h>

GameServer::GameServer(int port) :
        currentDirection(Direction::NONE),
        turretRotationDelta(0),
        buttonPressed(false)
{
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Bind address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Start listening
    if (listen(server_fd, 1) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server started on port " << port << std::endl;
}

GameServer::~GameServer() {
    close(client_fd);
    close(server_fd);
}

void GameServer::start() {
    addr_len = sizeof(client_addr);
    client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
    if (client_fd < 0) {
        perror("Accept failed");
        exit(EXIT_FAILURE);
    }

    std::cout << "Client connected: " << inet_ntoa(client_addr.sin_addr) << std::endl;

    // Start receiving input in a separate thread
    std::thread inputThread(&GameServer::receiveInput, this);
    inputThread.detach();
}

Direction GameServer::getCurrentDirection() const {
    return currentDirection;
}

int GameServer::getTurretRotationDelta() {
    int delta = turretRotationDelta;
    turretRotationDelta = 0;
    return delta;
}

bool GameServer::getButtonPressed() {
    bool pressed = buttonPressed;
    buttonPressed = false;
    return pressed;
}

void GameServer::receiveInput() {
    char buffer[32];

    // Send initial state request
    const char* init_request = "INIT";
    write(client_fd, init_request, strlen(init_request));

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytesRead = read(client_fd, buffer, sizeof(buffer));

        if (bytesRead <= 0) {
            std::cout << "Client disconnected." << std::endl;
            break;
        }

        std::string input(buffer);

        // Reset state
        currentDirection = Direction::NONE;
        turretRotationDelta = 0;
        buttonPressed = false;

        // Process input
        size_t start = 0;
        size_t end = input.find(',');

        while (end != std::string::npos) {
            processInputToken(input.substr(start, end - start));
            start = end + 1;
            end = input.find(',', start);
        }
        processInputToken(input.substr(start));
    }
}

void GameServer::processInputToken(const std::string& token) {
    if (token == "UP") currentDirection = Direction::UP;
    else if (token == "DOWN") currentDirection = Direction::DOWN;
    else if (token == "LEFT") currentDirection = Direction::LEFT;
    else if (token == "RIGHT") currentDirection = Direction::RIGHT;
    else if (token.find("ROT:") == 0) {
        turretRotationDelta += std::stoi(token.substr(4));
    }
    else if (token.find("BTN:") == 0) {
        buttonPressed = (token.substr(4) == "1");
    }
}

// Send tank health to client
void GameServer::sendTankHealth(int health) {
    if (client_fd > 0) {
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "HP:%d", health);

        // If send fails, this simply prints an error. For production code,
        // you might want to handle reconnection or mark the client as disconnected.
        if (send(client_fd, buffer, strlen(buffer), 0) == -1) {
            perror("Failed to send tank health");
        }
    }
}
