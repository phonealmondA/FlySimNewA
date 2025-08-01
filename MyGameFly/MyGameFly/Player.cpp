#include "Player.h"
#include "NetworkManager.h"  // Include after Player.h to get PlayerState definition
#include "GameConstants.h"
#include <SFML/Window/Keyboard.hpp>
#include <iostream>
#include <sstream>

Player::Player(int id, sf::Vector2f spawnPos, PlayerType playerType, const std::vector<Planet*>& planetList)
    : playerID(id),
    spawnPosition(spawnPos),
    type(playerType),
    planets(planetList),
    stateChanged(false),
    timeSinceLastStateSent(0.0f) {

    // Set default player name
    std::stringstream ss;
    ss << "Player " << (playerID + 1);  // Display as 1-indexed
    playerName = ss.str();

    // Initialize the VehicleManager at spawn position
    initializeVehicleManager();

    std::cout << "Created " << playerName << " (" << (type == PlayerType::LOCAL ? "LOCAL" : "REMOTE")
        << ") at position (" << spawnPos.x << ", " << spawnPos.y << ")" << std::endl;
}

void Player::initializeVehicleManager() {
    vehicleManager = std::make_unique<VehicleManager>(spawnPosition, planets);

    // Set up the rocket with nearby planets for physics
    if (vehicleManager->getRocket()) {
        vehicleManager->getRocket()->setNearbyPlanets(planets);
    }
}

void Player::update(float deltaTime) {
    // Update state send timer
    timeSinceLastStateSent += deltaTime;

    if (type == PlayerType::LOCAL) {
        // Handle local input for this player
        handleLocalInput(deltaTime);
    }
    // REMOTE players just run physics based on their current state

    // Update the vehicle regardless of player type
    if (vehicleManager) {
        vehicleManager->update(deltaTime);
    }
}

void Player::handleLocalInput(float deltaTime) {
    if (type != PlayerType::LOCAL) return;

    // Apply input directly to vehicle - no networking here!
    if (vehicleManager) {
        bool inputActive = false;

        // Different controls based on player ID
        bool thrust, reverseThrust, rotateLeft, rotateRight;

        if (playerID == 0) {
            // Player 1 (Host): Arrow keys
            thrust = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up);
            reverseThrust = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down);
            rotateLeft = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left);
            rotateRight = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right);
        }
        else {
            // Player 2+ (Clients): WASD keys
            thrust = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W);
            reverseThrust = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S);
            rotateLeft = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A);
            rotateRight = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D);
        }

        // Apply movement
        if (thrust) {
            vehicleManager->applyThrust(1.0f);
            inputActive = true;
        }
        if (reverseThrust) {
            vehicleManager->applyThrust(-0.5f);
            inputActive = true;
        }
        if (rotateLeft) {
            vehicleManager->rotate(-6.0f * deltaTime * 60.0f);
            inputActive = true;
        }
        if (rotateRight) {
            vehicleManager->rotate(6.0f * deltaTime * 60.0f);
            inputActive = true;
        }

        // Handle thrust level (number keys 0-9, =)
        float newThrustLevel = -1.0f; // -1 means no change
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num0)) newThrustLevel = 0.0f;
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num1)) newThrustLevel = 0.1f;
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num2)) newThrustLevel = 0.2f;
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num3)) newThrustLevel = 0.3f;
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num4)) newThrustLevel = 0.4f;
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num5)) newThrustLevel = 0.5f;
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num6)) newThrustLevel = 0.6f;
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num7)) newThrustLevel = 0.7f;
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num8)) newThrustLevel = 0.8f;
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num9)) newThrustLevel = 0.9f;
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Equal)) newThrustLevel = 1.0f;

        if (newThrustLevel >= 0.0f && vehicleManager->getRocket()) {
            vehicleManager->getRocket()->setThrustLevel(newThrustLevel);
            inputActive = true;
        }

        // Mark state as changed if any input was active
        if (inputActive) {
            stateChanged = true;
        }
    }
}

PlayerState Player::getState() const {
    PlayerState state;
    state.playerID = playerID;

    if (vehicleManager) {
        state.position = vehicleManager->getActiveVehicle()->getPosition();
        state.velocity = vehicleManager->getActiveVehicle()->getVelocity();
        state.isRocket = (vehicleManager->getActiveVehicleType() == VehicleType::ROCKET);

        if (state.isRocket && vehicleManager->getRocket()) {
            Rocket* rocket = vehicleManager->getRocket();
            state.rotation = rocket->getRotation();
            state.mass = rocket->getMass();
            state.thrustLevel = rocket->getThrustLevel();
            state.isOnGround = false;

            // ADD FUEL DATA TO STATE
            state.currentFuel = rocket->getCurrentFuel();
            state.maxFuel = rocket->getMaxFuel();
            state.isCollectingFuel = rocket->getIsCollectingFuel();
        }
        else if (!state.isRocket && vehicleManager->getCar()) {
            state.rotation = vehicleManager->getCar()->getRotation();
            state.mass = GameConstants::ROCKET_MASS;
            state.thrustLevel = 0.0f;
            state.isOnGround = vehicleManager->getCar()->isOnGround();

            // Cars don't use fuel (for now)
            state.currentFuel = GameConstants::ROCKET_MAX_FUEL;
            state.maxFuel = GameConstants::ROCKET_MAX_FUEL;
            state.isCollectingFuel = false;
        }
    }

    return state;
}

void Player::applyState(const PlayerState& state) {
    if (type != PlayerType::REMOTE) return; // Only remote players receive state updates

    if (vehicleManager) {
        // Apply position and velocity
        vehicleManager->getActiveVehicle()->setVelocity(state.velocity);

        // Apply vehicle-specific state
        if (state.isRocket && vehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
            Rocket* rocket = vehicleManager->getRocket();
            if (rocket) {
                rocket->setPosition(state.position);
                // Apply fuel state for remote players (for visual consistency)
                rocket->setFuel(state.currentFuel);
                // Don't override thrust level for remote players in state sync
            }
        }
        else if (!state.isRocket && vehicleManager->getActiveVehicleType() == VehicleType::CAR) {
            // Car state application would go here
        }

        // Handle vehicle type mismatch (transformation needed)
        if ((state.isRocket && vehicleManager->getActiveVehicleType() != VehicleType::ROCKET) ||
            (!state.isRocket && vehicleManager->getActiveVehicleType() != VehicleType::CAR)) {
            vehicleManager->switchVehicle();
        }
    }
}

bool Player::shouldSendState() const {
    // Send state if enough time has passed OR if state changed significantly
    return (type == PlayerType::LOCAL) &&
        (timeSinceLastStateSent >= STATE_SEND_INTERVAL || stateChanged);
}

void Player::requestTransform() {
    if (type == PlayerType::LOCAL && vehicleManager) {
        vehicleManager->switchVehicle();
        stateChanged = true; // Mark for immediate state sync
    }
}

sf::Vector2f Player::getPosition() const {
    if (vehicleManager) {
        return vehicleManager->getActiveVehicle()->getPosition();
    }
    return spawnPosition;
}

sf::Vector2f Player::getVelocity() const {
    if (vehicleManager) {
        return vehicleManager->getActiveVehicle()->getVelocity();
    }
    return sf::Vector2f(0, 0);
}

// FUEL SYSTEM GETTERS
float Player::getCurrentFuel() const {
    if (vehicleManager && vehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
        return vehicleManager->getRocket()->getCurrentFuel();
    }
    return GameConstants::ROCKET_MAX_FUEL; // Cars have "infinite" fuel
}

float Player::getMaxFuel() const {
    if (vehicleManager && vehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
        return vehicleManager->getRocket()->getMaxFuel();
    }
    return GameConstants::ROCKET_MAX_FUEL;
}

float Player::getFuelPercentage() const {
    if (vehicleManager && vehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
        return vehicleManager->getRocket()->getFuelPercentage();
    }
    return 100.0f; // Cars have "full" fuel
}

bool Player::canThrust() const {
    if (vehicleManager && vehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
        return vehicleManager->getRocket()->canThrust();
    }
    return true; // Cars can always "thrust" (accelerate)
}

bool Player::isCollectingFuel() const {
    if (vehicleManager && vehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
        return vehicleManager->getRocket()->getIsCollectingFuel();
    }
    return false; // Cars don't collect fuel
}

void Player::draw(sf::RenderWindow& window) {
    if (vehicleManager) {
        vehicleManager->draw(window);
    }
}

void Player::drawWithConstantSize(sf::RenderWindow& window, float zoomLevel) {
    if (vehicleManager) {
        vehicleManager->drawWithConstantSize(window, zoomLevel);
    }
}

void Player::drawVelocityVector(sf::RenderWindow& window, float scale) {
    if (vehicleManager) {
        vehicleManager->drawVelocityVector(window, scale);
    }
}

void Player::drawFuelCollectionIndicator(sf::RenderWindow& window) {
    if (!vehicleManager || vehicleManager->getActiveVehicleType() != VehicleType::ROCKET) {
        return; // Only rockets collect fuel
    }

    Rocket* rocket = vehicleManager->getRocket();
    if (!rocket || !rocket->getIsCollectingFuel()) {
        return; // Not currently collecting fuel
    }

    // Draw a visual indicator showing fuel collection
    Planet* sourcePlanet = rocket->getFuelSourcePlanet();
    if (sourcePlanet) {
        // Draw active fuel collection ring around the source planet
        sourcePlanet->drawFuelCollectionRing(window, true);

        // Draw a line connecting rocket to planet to show fuel transfer
        sf::VertexArray fuelLine(sf::PrimitiveType::LineStrip);

        sf::Vertex startVertex;
        startVertex.position = rocket->getPosition();
        startVertex.color = GameConstants::FUEL_RING_ACTIVE_COLOR;
        fuelLine.append(startVertex);

        sf::Vertex endVertex;
        endVertex.position = sourcePlanet->getPosition();
        endVertex.color = GameConstants::FUEL_RING_ACTIVE_COLOR;
        fuelLine.append(endVertex);

        window.draw(fuelLine);
    }
}

void Player::drawPlayerLabel(sf::RenderWindow& window, const sf::Font& font) {
    if (!vehicleManager) return;

    sf::Text label(font);
    label.setString(playerName);
    label.setCharacterSize(14);
    label.setFillColor(sf::Color::White);

    sf::Vector2f playerPos = getPosition();
    sf::FloatRect textBounds = label.getLocalBounds();
    label.setPosition(sf::Vector2f(playerPos.x - textBounds.size.x / 2, playerPos.y - 40.0f));

    window.draw(label);
}

void Player::respawnAtPosition(sf::Vector2f newSpawnPos) {
    spawnPosition = newSpawnPos;

    if (vehicleManager) {
        vehicleManager = std::make_unique<VehicleManager>(spawnPosition, planets);
        if (vehicleManager->getRocket()) {
            vehicleManager->getRocket()->setNearbyPlanets(planets);
        }
    }

    stateChanged = true; // Force state sync after respawn
    std::cout << playerName << " respawned at (" << newSpawnPos.x << ", " << newSpawnPos.y << ")" << std::endl;
}