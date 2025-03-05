#include "../include/GameServer.h"
#include <cstring>
#include <arpa/inet.h>
#include <thread>


// main constructor -> Initializes the server socket and binds it to the specified port.
GameServer::GameServer(int port) : currentDirection(Direction::NONE) {
    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Bind address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    addr_len = sizeof(client_addr);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Start listening
    if (listen(server_fd, 1) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server started on port " << port << ". Waiting for a client..." << std::endl;
}

GameServer::~GameServer() {
    close(client_fd);
    close(server_fd);
}

// Accepts a client connection
void GameServer::start() {
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

// Reads movement data from the client
void GameServer::receiveInput() {
    char buffer[1024];

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytesRead = read(client_fd, buffer, sizeof(buffer));

        if (bytesRead <= 0) {
            std::cout << "Client disconnected." << std::endl;
            break;
        }

        std::string input(buffer);
        if (input == "UP") currentDirection = Direction::UP;
        else if (input == "DOWN") currentDirection = Direction::DOWN;
        else if (input == "LEFT") currentDirection = Direction::LEFT;
        else if (input == "RIGHT") currentDirection = Direction::RIGHT;
        else currentDirection = Direction::NONE;

        std::cout << "Received direction: " << input << std::endl;
    }
}

// Returns the current direction
Direction GameServer::getCurrentDirection() const {
    return currentDirection;
}
