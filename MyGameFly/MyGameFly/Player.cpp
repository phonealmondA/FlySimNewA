#include "Player.h"
#include "NetworkManager.h"  // For PlayerInput and PlayerState structs
#include "GameConstants.h"
#include <SFML/Window/Keyboard.hpp>
#include <iostream>
#include <sstream>

Player::Player(int id, sf::Vector2f spawnPos, PlayerType playerType, const std::vector<Planet*>& planetList)
    : playerID(id),
    spawnPosition(spawnPos),
    type(playerType),
    planets(planetList),
    inputChanged(false) {

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
    if (type == PlayerType::LOCAL) {
        // Handle local input for this player
        handleLocalInput(deltaTime);
    }
    // For REMOTE players, input is applied via applyNetworkInput()

    // Update the vehicle regardless of player type
    if (vehicleManager) {
        vehicleManager->update(deltaTime);
    }
}

void Player::handleLocalInput(float deltaTime) {
    if (type != PlayerType::LOCAL) return;

    // Store previous input state to detect changes
    PlayerInput previousInput = currentInput;

    // Update input from keyboard
    updateInputFromKeyboard();

    // Apply input to vehicle
    if (vehicleManager) {
        if (currentInput.thrust) {
            vehicleManager->applyThrust(1.0f);
        }
        if (currentInput.reverseThrust) {
            vehicleManager->applyThrust(-0.5f);
        }
        if (currentInput.rotateLeft) {
            vehicleManager->rotate(-6.0f * deltaTime * 60.0f);
        }
        if (currentInput.rotateRight) {
            vehicleManager->rotate(6.0f * deltaTime * 60.0f);
        }

        // Set thrust level on the rocket
        if (vehicleManager->getRocket()) {
            vehicleManager->getRocket()->setThrustLevel(currentInput.thrustLevel);
        }
    }

    // Check if input changed (for networking)
    inputChanged = (
        previousInput.thrust != currentInput.thrust ||
        previousInput.reverseThrust != currentInput.reverseThrust ||
        previousInput.rotateLeft != currentInput.rotateLeft ||
        previousInput.rotateRight != currentInput.rotateRight ||
        previousInput.transform != currentInput.transform ||
        std::abs(previousInput.thrustLevel - currentInput.thrustLevel) > 0.01f
        );
}

void Player::updateInputFromKeyboard() {
    currentInput.playerID = playerID;
    currentInput.sequenceNumber++; // Increment for networking

    // Movement input (same for all players for now - can be customized per player later)
    currentInput.thrust = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up);
    currentInput.reverseThrust = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down);
    currentInput.rotateLeft = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left);
    currentInput.rotateRight = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right);

    // Transform input (L key)
    currentInput.transform = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::L);

    // Thrust level input (number keys 0-9, =)
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num0))
        currentInput.thrustLevel = 0.0f;
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num1))
        currentInput.thrustLevel = 0.1f;
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num2))
        currentInput.thrustLevel = 0.2f;
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num3))
        currentInput.thrustLevel = 0.3f;
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num4))
        currentInput.thrustLevel = 0.4f;
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num5))
        currentInput.thrustLevel = 0.5f;
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num6))
        currentInput.thrustLevel = 0.6f;
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num7))
        currentInput.thrustLevel = 0.7f;
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num8))
        currentInput.thrustLevel = 0.8f;
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num9))
        currentInput.thrustLevel = 0.9f;
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Equal))
        currentInput.thrustLevel = 1.0f;
    // If no thrust key is pressed, keep the current thrust level
}

void Player::applyNetworkInput(const PlayerInput& input) {
    if (type != PlayerType::REMOTE) return;

    currentInput = input;

    // Apply input to vehicle
    if (vehicleManager) {
        if (input.thrust) {
            vehicleManager->applyThrust(1.0f);
        }
        if (input.reverseThrust) {
            vehicleManager->applyThrust(-0.5f);
        }
        // Note: Rotation will be handled by the authoritative host

        // Set thrust level
        if (vehicleManager->getRocket()) {
            vehicleManager->getRocket()->setThrustLevel(input.thrustLevel);
        }

        // Handle transform
        if (input.transform) {
            vehicleManager->switchVehicle();
        }
    }
}

PlayerInput Player::getCurrentInput() const {
    return currentInput;
}

PlayerState Player::getState() const {
    PlayerState state;
    state.playerID = playerID;

    if (vehicleManager) {
        state.position = vehicleManager->getActiveVehicle()->getPosition();
        state.velocity = vehicleManager->getActiveVehicle()->getVelocity();
        state.isRocket = (vehicleManager->getActiveVehicleType() == VehicleType::ROCKET);

        if (state.isRocket && vehicleManager->getRocket()) {
            state.rotation = vehicleManager->getRocket()->getRotation();
            state.mass = vehicleManager->getRocket()->getMass();
            state.thrustLevel = vehicleManager->getRocket()->getThrustLevel();
            state.isOnGround = false; // Rockets don't have ground state
        }
        else if (!state.isRocket && vehicleManager->getCar()) {
            state.rotation = vehicleManager->getCar()->getRotation();
            state.mass = GameConstants::ROCKET_MASS; // Use default mass for cars
            state.thrustLevel = 0.0f; // Cars don't have thrust
            state.isOnGround = vehicleManager->getCar()->isOnGround();
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
                // Note: Don't override local thrust level for remote players
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

void Player::setThrustLevel(float level) {
    currentInput.thrustLevel = std::max(0.0f, std::min(1.0f, level));
    inputChanged = true;

    if (vehicleManager && vehicleManager->getRocket()) {
        vehicleManager->getRocket()->setThrustLevel(currentInput.thrustLevel);
    }
}

float Player::getThrustLevel() const {
    return currentInput.thrustLevel;
}

void Player::applyThrust(float amount) {
    if (vehicleManager) {
        vehicleManager->applyThrust(amount);
    }
}

void Player::rotate(float amount) {
    if (vehicleManager) {
        vehicleManager->rotate(amount);
    }
}

void Player::requestTransform() {
    currentInput.transform = true;
    inputChanged = true;

    if (type == PlayerType::LOCAL && vehicleManager) {
        vehicleManager->switchVehicle();
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

void Player::drawPlayerLabel(sf::RenderWindow& window, const sf::Font& font) {
    if (!vehicleManager) return;

    // Create text label showing player name
    sf::Text label;
    label.setFont(font);
    label.setString(playerName);
    label.setCharacterSize(14);
    label.setFillColor(sf::Color::White);

    // Position label above the player
    sf::Vector2f playerPos = getPosition();
    sf::FloatRect textBounds = label.getLocalBounds();
    label.setPosition(playerPos.x - textBounds.size.x / 2, playerPos.y - 40.0f);

    window.draw(label);
}

void Player::respawnAtPosition(sf::Vector2f newSpawnPos) {
    spawnPosition = newSpawnPos;

    if (vehicleManager) {
        // Reset vehicle to rocket mode at new position
        vehicleManager = std::make_unique<VehicleManager>(spawnPosition, planets);
        if (vehicleManager->getRocket()) {
            vehicleManager->getRocket()->setNearbyPlanets(planets);
        }
    }

    std::cout << playerName << " respawned at (" << newSpawnPos.x << ", " << newSpawnPos.y << ")" << std::endl;
}