#include "../include/GameRender.h"
#include <iostream>
#include <cstdlib>
#include <ctime>

GameRender::GameRender()
        : window(sf::VideoMode(1024, 768), "Tank Game")
{

    // Generate a grass background texture.
    sf::RenderTexture grassRenderTexture;
    if (!grassRenderTexture.create(1024, 768)) {
        std::cerr << "Failed to create render texture for grass background." << std::endl;
    }
    // Base green color for grass.
    grassRenderTexture.clear(sf::Color(50, 205, 50)); // a lime-green tone

    // Seed random generator.
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    sf::CircleShape clump;
    clump.setRadius(2.f);
    clump.setOrigin(2.f, 2.f);

    // Draw 500 small clumps with slight color variations.
    for (int i = 0; i < 500; ++i) {
        float x = static_cast<float>(std::rand() % 1024);
        float y = static_cast<float>(std::rand() % 768);
        clump.setPosition(x, y);
        int variation = std::rand() % 30 - 15;
        sf::Color clumpColor(
                std::max(0, 50 + variation),
                std::min(255, 205 + variation),
                std::max(0, 50 + variation)
        );
        clump.setFillColor(clumpColor);
        grassRenderTexture.draw(clump);
    }
    grassRenderTexture.display();
    backgroundTexture = grassRenderTexture.getTexture();
    backgroundSprite.setTexture(backgroundTexture);
    // (No scaling needed if the texture size matches the window size.)

    // Load the tank body and turret images.
    if (!bodyTexture.loadFromFile("Assets/body.png")) {
        std::cerr << "Failed to load body.png" << std::endl;
    }
    if (!turretTexture.loadFromFile("Assets/turret.png")) {
        std::cerr << "Failed to load turret.png" << std::endl;
    }
    bodySprite.setTexture(bodyTexture);
    turretSprite.setTexture(turretTexture);

    // Set the origin to the center so rotations look natural.
    bodySprite.setOrigin(bodyTexture.getSize().x / 2.0f, bodyTexture.getSize().y / 2.0f);
    turretSprite.setOrigin(turretTexture.getSize().x / 2.0f, turretTexture.getSize().y / 2.0f);

    // Scale the images up 4× compared to the previous size.
    bodySprite.setScale(0.20f, 0.20f);
    turretSprite.setScale(0.20f, 0.20f);
}

GameRender::~GameRender() {
    window.close();
}

void GameRender::run(GameState & state, Direction & currentDir) {
    while (window.isOpen()) {
        // Handle window events.
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        // Draw the grass background.
        window.clear();
        window.draw(backgroundSprite);

        // Update sprite positions from the current game state.
        const Tank & tank = state.getTank();
        bodySprite.setPosition(static_cast<float>(tank.x), static_cast<float>(tank.y));
        turretSprite.setPosition(static_cast<float>(tank.x), static_cast<float>(tank.y));

        // Set tank body rotation based on direction
        auto getAngleForDirection = [](Direction dir) -> float {
            switch (dir) {
                case Direction::UP:    return 0.0f;
                case Direction::RIGHT: return 90.0f;
                case Direction::DOWN:  return 180.0f;
                case Direction::LEFT:  return 270.0f;
                default: return 90.0f; // Default to facing right
            }
        };
        bodySprite.setRotation(getAngleForDirection(currentDir));

        // Set turret rotation based on GameState's turret angle
        turretSprite.setRotation(state.getTurretAngle());

        // Draw the tank
        window.draw(bodySprite);
        window.draw(turretSprite);
        window.display();

        // ~60 FPS
        sf::sleep(sf::milliseconds(16));
    }
}
