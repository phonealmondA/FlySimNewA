#include "SplitScreenManager.h"
#include "GameConstants.h"
#include <SFML/Window/Keyboard.hpp>
#include <cmath>
#include <algorithm>
#include "SatelliteManager.h"
#include <limits>

SplitScreenManager::SplitScreenManager(sf::Vector2f player1Pos, sf::Vector2f player2Pos, const std::vector<Planet*>& planetList, SatelliteManager* satManager)
    : planets(planetList), satelliteManager(satManager), player1LKeyPressed(false), player2LKeyPressed(false),
    player1TKeyPressed(false), player2TKeyPressed(false) {

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
            // Player 1: L key (vehicle transform)
            if (keyEvent->code == sf::Keyboard::Key::L && !player1LKeyPressed) {
                player1LKeyPressed = true;
                player1->switchVehicle();
            }
            // Player 2: K key (vehicle transform)
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

    // Handle satellite conversion inputs
    handleSatelliteConversionInputs(event);
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


void SplitScreenManager::handleSatelliteConversionInputs(const sf::Event& event) {
    if (!satelliteManager) return;

    if (event.is<sf::Event::KeyPressed>()) {
        const auto* keyEvent = event.getIf<sf::Event::KeyPressed>();
        if (keyEvent) {
            // Player 1: T key (satellite conversion)
            if (keyEvent->code == sf::Keyboard::Key::T && !player1TKeyPressed) {
                player1TKeyPressed = true;
                if (canPlayerConvertToSatellite(0)) {
                    // Convert Player 1's rocket to satellite
                    Rocket* rocket = player1->getRocket();
                    if (rocket && satelliteManager->canConvertRocketToSatellite(rocket)) {
                        SatelliteConversionConfig config = satelliteManager->getOptimalConversionConfig(rocket);
                        config.customName = "P1-SAT-" + std::to_string(satelliteManager->getSatelliteCount() + 1);

                        int satelliteID = satelliteManager->createSatelliteFromRocket(rocket, config);
                        if (satelliteID >= 0) {
                            // Spawn new rocket for Player 1
                            sf::Vector2f newRocketPos = findNearestPlanetSurface(rocket->getPosition(), 0);
                            player1 = std::make_unique<VehicleManager>(newRocketPos, planets);
                            updateSatelliteManager();
                            /*std::cout << "Player 1 converted rocket to satellite (ID: " << satelliteID << ")" << std::endl;*/
                        }
                    }
                }
            }
            // Player 2: Y key (satellite conversion)
            else if (keyEvent->code == sf::Keyboard::Key::Y && !player2TKeyPressed) {
                player2TKeyPressed = true;
                if (canPlayerConvertToSatellite(1)) {
                    // Convert Player 2's rocket to satellite
                    Rocket* rocket = player2->getRocket();
                    if (rocket && satelliteManager->canConvertRocketToSatellite(rocket)) {
                        SatelliteConversionConfig config = satelliteManager->getOptimalConversionConfig(rocket);
                        config.customName = "P2-SAT-" + std::to_string(satelliteManager->getSatelliteCount() + 1);

                        int satelliteID = satelliteManager->createSatelliteFromRocket(rocket, config);
                        if (satelliteID >= 0) {
                            // Spawn new rocket for Player 2
                            sf::Vector2f newRocketPos = findNearestPlanetSurface(rocket->getPosition(), 1);
                            player2 = std::make_unique<VehicleManager>(newRocketPos, planets);
                            updateSatelliteManager();
                            /*std::cout << "Player 2 converted rocket to satellite (ID: " << satelliteID << ")" << std::endl;*/
                        }
                    }
                }
            }
        }
    }

    if (event.is<sf::Event::KeyReleased>()) {
        const auto* keyEvent = event.getIf<sf::Event::KeyReleased>();
        if (keyEvent) {
            if (keyEvent->code == sf::Keyboard::Key::T) {
                player1TKeyPressed = false;
            }
            else if (keyEvent->code == sf::Keyboard::Key::Y) {
                player2TKeyPressed = false;
            }
        }
    }
}

bool SplitScreenManager::canPlayerConvertToSatellite(int playerIndex) const {
    if (!satelliteManager) return false;

    VehicleManager* player = (playerIndex == 0) ? player1.get() : player2.get();
    if (!player || player->getActiveVehicleType() != VehicleType::ROCKET) return false;

    Rocket* rocket = player->getRocket();
    return rocket && satelliteManager->canConvertRocketToSatellite(rocket);
}

sf::Vector2f SplitScreenManager::findNearestPlanetSurface(sf::Vector2f position, int playerIndex) const {
    Planet* nearestPlanet = nullptr;
    float minDistance = std::numeric_limits<float>::max();

    for (auto* planet : planets) {
        if (!planet) continue;
        sf::Vector2f direction = planet->getPosition() - position;
        float dist = std::sqrt(direction.x * direction.x + direction.y * direction.y);
        if (dist < minDistance) {
            minDistance = dist;
            nearestPlanet = planet;
        }
    }

    if (nearestPlanet) {
        // Offset spawn positions slightly for each player to avoid overlap
        float angleOffset = (playerIndex == 0) ? 0.0f : 3.14159f; // Player 2 on opposite side
        sf::Vector2f direction = position - nearestPlanet->getPosition();
        float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
        if (length > 0) {
            direction /= length; // normalize
        }

        // Apply angle offset
        float cosOffset = std::cos(angleOffset);
        float sinOffset = std::sin(angleOffset);
        sf::Vector2f offsetDirection(
            direction.x * cosOffset - direction.y * sinOffset,
            direction.x * sinOffset + direction.y * cosOffset
        );

        return nearestPlanet->getPosition() + offsetDirection * (nearestPlanet->getRadius() + GameConstants::ROCKET_SIZE + 10.0f);
    }

    // Fallback positions
    return (playerIndex == 0) ?
        sf::Vector2f(GameConstants::MAIN_PLANET_X, GameConstants::MAIN_PLANET_Y - 150.0f) :
        sf::Vector2f(GameConstants::MAIN_PLANET_X, GameConstants::MAIN_PLANET_Y + 150.0f);
}

void SplitScreenManager::updateSatelliteManager() {
    if (!satelliteManager) return;

    // Update satellite manager with current rocket references
    std::vector<Rocket*> activeRockets;

    if (player1->getActiveVehicleType() == VehicleType::ROCKET) {
        activeRockets.push_back(player1->getRocket());
    }
    if (player2->getActiveVehicleType() == VehicleType::ROCKET) {
        activeRockets.push_back(player2->getRocket());
    }

    satelliteManager->setNearbyRockets(activeRockets);
}