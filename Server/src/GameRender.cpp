#include "../include/GameRender.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <mutex>
#include "Shutdown.h"

GameRender::GameRender()
        : window(sf::VideoMode(1024, 768), "Tank Game") {

    // Generate a grass background texture.
    sf::RenderTexture grassRenderTexture;
    if (!grassRenderTexture.create(1024, 768)) {
        std::cerr << "Failed to create render texture for grass background." << std::endl;
    }
    // Base green color for grass.
    grassRenderTexture.clear(sf::Color(50, 205, 50));

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

    // Projectile shape
    projectileShape.setRadius(5.f);
    projectileShape.setFillColor(sf::Color::Red);
    projectileShape.setOrigin(2.5f, 2.5f);

    // Hit effect
    hitEffect.setSize(sf::Vector2f(50, 50));
    hitEffect.setFillColor(sf::Color(255, 0, 0, 150));
    hitEffect.setOrigin(25, 25);

    // For Game Over Text
    if (!gameOverFont.loadFromFile("Assets/arial.ttf")) {
        std::cerr << "Failed to load font for game over text" << std::endl;
    }
    gameOverText.setFont(gameOverFont);
    gameOverText.setCharacterSize(100);
    gameOverText.setFillColor(sf::Color::Red);
    gameOverText.setString("GAME OVER");

    overlay.setSize(sf::Vector2f(1024.f, 768.f));
    overlay.setFillColor(sf::Color(0, 0, 0, 128));
}

void GameRender::run(GameState &state, Direction &lastDir, std::atomic<bool>& running) {
    while (window.isOpen() && running && !ShutdownModule::isShutdownRequested()) {
        float dt = deltaClock.restart().asSeconds();

        // Handle window events.
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                running = false;
                ShutdownModule::requestShutdown();
                break;
            }
        }

        window.clear();
        window.draw(backgroundSprite);

        {
            // Lock the game state during update and drawing.
            std::lock_guard<std::recursive_mutex> lock(state.getMutex());

            // Only draw tank if player is alive
            if (state.isPlayerAlive()) {
                const Tank &tank = state.getTank();
                bodySprite.setPosition(tank.x, tank.y);
                turretSprite.setPosition(tank.x, tank.y);

                auto getAngleForDirection = [](Direction dir) -> float {
                    switch (dir) {
                        case Direction::UP:
                            return 0.0f;
                        case Direction::RIGHT:
                            return 90.0f;
                        case Direction::DOWN:
                            return 180.0f;
                        case Direction::LEFT:
                            return 270.0f;
                        default:
                            return 90.0f;
                    }
                };

                bodySprite.setRotation(getAngleForDirection(lastDir));
                turretSprite.setRotation(state.getTurretAngle());

                // Draw hit effect if active
                if (state.getTankHitEffect() > 0) {
                    hitEffect.setPosition(tank.x, tank.y);
                    window.draw(hitEffect);
                }

                window.draw(bodySprite);
                window.draw(turretSprite);
            }

            // Draw projectiles.
            for (const auto &p: state.getProjectiles()) {
                projectileShape.setPosition(p.x, p.y);
                projectileShape.setFillColor(p.isEnemyProjectile ? sf::Color::Yellow : sf::Color::Red);
                window.draw(projectileShape);
            }

            // Update and draw enemies.
            for (const auto &enemy: state.getEnemies()) {
                enemy->update(dt);
                enemy->draw(window);
            }
        }

        if (!state.isPlayerAlive()) {
            // Draw a semi-transparent overlay
            overlay.setPosition(0.f, 0.f);
            window.draw(overlay);

            // Center the "GAME OVER" text
            sf::FloatRect textRect = gameOverText.getLocalBounds();
            gameOverText.setOrigin(textRect.width / 2.0f, textRect.height / 2.0f);
            gameOverText.setPosition(1024.f / 2.0f, 768.f / 2.0f);
            window.draw(gameOverText);
        }

        window.display();
        sf::sleep(sf::milliseconds(16));
    }

    // Clean up SFML resources while OpenGL context is still valid
    if (window.isOpen()) {
        window.close();
    }

    // Explicitly release SFML resources
    gameOverText = sf::Text();  // Releases font reference
    bodySprite = sf::Sprite();  // Releases texture references
    turretSprite = sf::Sprite();
    backgroundSprite = sf::Sprite();
}

GameRender::~GameRender() {
    window.close();
}