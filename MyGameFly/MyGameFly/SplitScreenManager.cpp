#include "SplitScreenManager.h"
#include "GameConstants.h"
#include <SFML/Window/Keyboard.hpp>
#include <cmath>
#include <algorithm>

SplitScreenManager::SplitScreenManager(sf::Vector2f player1Pos, sf::Vector2f player2Pos, const std::vector<Planet*>& planetList)
    : planets(planetList), player1LKeyPressed(false), player2LKeyPressed(false) {

    // Create Player 1 (White rocket)
    player1 = std::make_unique<VehicleManager>(player1Pos, planets);

    // Create Player 2 (Red rocket) - spawn on opposite side of planet
    player2 = std::make_unique<VehicleManager>(player2Pos, planets);

    // Set different colors for visual distinction
    // Note: You might need to modify VehicleManager/Rocket to support color changes
    // For now, we'll rely on position differences
}

void SplitScreenManager::handlePlayer1Input(float deltaTime) {
    // Player 1 Controls: Arrow Keys
    // Note: Thrust level is handled globally in main.cpp

    // Movement controls (Arrow Keys)
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
        player1->applyThrust(1.0f);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
        player1->applyThrust(-0.5f);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
        player1->rotate(-6.0f * deltaTime * 60.0f);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
        player1->rotate(6.0f * deltaTime * 60.0f);
}

void SplitScreenManager::handlePlayer2Input(float deltaTime) {
    // Player 2 Controls: WASD
    // Note: Thrust level is handled globally in main.cpp

    // Movement controls (WASD)
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
        player2->applyThrust(1.0f);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
        player2->applyThrust(-0.5f);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
        player2->rotate(-6.0f * deltaTime * 60.0f);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
        player2->rotate(6.0f * deltaTime * 60.0f);
}

void SplitScreenManager::setSyncedThrustLevel(float level) {
    // Set the same thrust level for both players
    player1->getRocket()->setThrustLevel(level);
    player2->getRocket()->setThrustLevel(level);
}

void SplitScreenManager::handleTransformInputs(const sf::Event& event) {
    if (event.is<sf::Event::KeyPressed>()) {
        const auto* keyEvent = event.getIf<sf::Event::KeyPressed>();
        if (keyEvent) {
            // Player 1: L key
            if (keyEvent->code == sf::Keyboard::Key::L && !player1LKeyPressed) {
                player1LKeyPressed = true;
                player1->switchVehicle();
            }
            // Player 2: K key
            else if (keyEvent->code == sf::Keyboard::Key::K && !player2LKeyPressed) {
                player2LKeyPressed = true;
                player2->switchVehicle();
            }
        }
    }

    if (event.is<sf::Event::KeyReleased>()) {
        const auto* keyEvent = event.getIf<sf::Event::KeyReleased>();
        if (keyEvent) {
            if (keyEvent->code == sf::Keyboard::Key::L) {
                player1LKeyPressed = false;
            }
            else if (keyEvent->code == sf::Keyboard::Key::K) {
                player2LKeyPressed = false;
            }
        }
    }
}

void SplitScreenManager::update(float deltaTime) {
    // Update both players
    player1->update(deltaTime);
    player2->update(deltaTime);
}

void SplitScreenManager::draw(sf::RenderWindow& window) {
    // Draw both players
    player1->draw(window);
    player2->draw(window);
}

void SplitScreenManager::drawWithConstantSize(sf::RenderWindow& window, float zoomLevel) {
    // Draw both players with constant size
    player1->drawWithConstantSize(window, zoomLevel);
    player2->drawWithConstantSize(window, zoomLevel);
}

void SplitScreenManager::drawVelocityVectors(sf::RenderWindow& window, float scale) {
    // Draw velocity vectors for both players
    player1->drawVelocityVector(window, scale);
    player2->drawVelocityVector(window, scale);

    // Draw gravity force vectors if in rocket mode
    if (player1->getActiveVehicleType() == VehicleType::ROCKET) {
        player1->getRocket()->drawGravityForceVectors(window, planets, GameConstants::GRAVITY_VECTOR_SCALE);
    }
    if (player2->getActiveVehicleType() == VehicleType::ROCKET) {
        player2->getRocket()->drawGravityForceVectors(window, planets, GameConstants::GRAVITY_VECTOR_SCALE);
    }
}

sf::Vector2f SplitScreenManager::getCenterPoint() {
    // Calculate the center point between both players
    sf::Vector2f player1Pos = player1->getActiveVehicle()->getPosition();
    sf::Vector2f player2Pos = player2->getActiveVehicle()->getPosition();

    return sf::Vector2f(
        (player1Pos.x + player2Pos.x) / 2.0f,
        (player1Pos.y + player2Pos.y) / 2.0f
    );
}

float SplitScreenManager::getRequiredZoomToShowBothPlayers(sf::Vector2u windowSize) {
    sf::Vector2f player1Pos = player1->getActiveVehicle()->getPosition();
    sf::Vector2f player2Pos = player2->getActiveVehicle()->getPosition();

    // Calculate distance between players
    float distance = std::sqrt(
        std::pow(player2Pos.x - player1Pos.x, 2) +
        std::pow(player2Pos.y - player1Pos.y, 2)
    );

    // Add some padding and calculate required zoom
    float padding = 200.0f;
    float requiredWidth = distance + padding;
    float requiredHeight = requiredWidth * (static_cast<float>(windowSize.y) / static_cast<float>(windowSize.x));

    // Calculate zoom based on window size
    float zoomX = requiredWidth / static_cast<float>(windowSize.x);
    float zoomY = requiredHeight / static_cast<float>(windowSize.y);

    // Use the larger zoom to ensure both players are visible
    float zoom = std::max(zoomX, zoomY);

    // Clamp zoom between reasonable limits
    return std::max(1.0f, std::min(zoom, 50.0f));
}