#include "Player.h"
#include "NetworkManager.h"  // Include after Player.h to get PlayerState definition
#include "GameConstants.h"
#include "Rocket.h"  // FIXED: Include complete Rocket definition
#include <SFML/Window/Keyboard.hpp>
#include <iostream>
#include <sstream>
#include <limits>

Player::Player(int id, sf::Vector2f spawnPos, PlayerType playerType, const std::vector<Planet*>& planetList, SatelliteManager* satManager)
    : playerID(id),
    spawnPosition(spawnPos),
    type(playerType),
    planets(planetList),
    satelliteManager(satManager),
    stateChanged(false),
    timeSinceLastStateSent(0.0f),
    fuelIncreaseKeyPressed(false),
    fuelDecreaseKeyPressed(false),
    satelliteConversionKeyPressed(false),
    networkManager(nullptr),
    isNetworkMultiplayerMode(false),
    lastNetworkSyncTime(0.0f),
    hasPendingPlanetUpdates(false) {

    // Set default player name
    std::stringstream ss;
    ss << "Player " << (playerID + 1);  // Display as 1-indexed
    playerName = ss.str();

    // Initialize planet references for network sync
    originalPlanets = planetList;

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
    lastNetworkSyncTime += deltaTime;

    if (type == PlayerType::LOCAL) {
        // Handle local input for this player
        handleLocalInput(deltaTime);

        // Handle network synchronization for local player
        if (isNetworkMultiplayerMode) {
            syncWithNetwork(deltaTime);
        }
    }
    // REMOTE players just run physics based on their current state

    // Update the vehicle regardless of player type
    if (vehicleManager) {
        vehicleManager->update(deltaTime);

        // Update state tracking from vehicle
        if (type == PlayerType::LOCAL) {
            updateStateFromVehicle();
        }
    }

    // Update rocket targeting for network multiplayer
    if (isNetworkMultiplayerMode && satelliteManager) {
        updateRocketTargetingForNetwork();
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

        // MANUAL FUEL TRANSFER INPUT HANDLING
        handleFuelTransferInput(deltaTime);

        // Mark state as changed if any input was active
        if (inputActive) {
            stateChanged = true;
        }
    }
}

void Player::handleFuelTransferInput(float deltaTime) {
    if (!vehicleManager || vehicleManager->getActiveVehicleType() != VehicleType::ROCKET) {
        return; // Only rockets can transfer fuel
    }

    Rocket* rocket = vehicleManager->getRocket();
    if (!rocket) return;

    // Check current key states
    bool fuelIncreasePressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Period);  // '.' key
    bool fuelDecreasePressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Comma);   // ',' key

    // Handle fuel increase (take fuel from planet)
    if (fuelIncreasePressed && !fuelIncreaseKeyPressed) {
        // Key just pressed - start fuel transfer
        float transferRate = GameConstants::MANUAL_FUEL_TRANSFER_RATE * rocket->getThrustLevel() * GameConstants::FUEL_TRANSFER_THRUST_MULTIPLIER;
        if (transferRate < GameConstants::MANUAL_FUEL_TRANSFER_RATE * 0.1f) {
            transferRate = GameConstants::MANUAL_FUEL_TRANSFER_RATE * 0.1f; // Minimum 10% rate
        }

        rocket->startFuelTransferIn(transferRate);
        stateChanged = true;
    }
    else if (!fuelIncreasePressed && fuelIncreaseKeyPressed) {
        // Key just released - stop fuel transfer
        rocket->stopFuelTransfer();
        stateChanged = true;
    }

    // Handle fuel decrease (give fuel to planet)
    if (fuelDecreasePressed && !fuelDecreaseKeyPressed) {
        // Key just pressed - start fuel transfer
        float transferRate = GameConstants::MANUAL_FUEL_TRANSFER_RATE * rocket->getThrustLevel() * GameConstants::FUEL_TRANSFER_THRUST_MULTIPLIER;
        if (transferRate < GameConstants::MANUAL_FUEL_TRANSFER_RATE * 0.1f) {
            transferRate = GameConstants::MANUAL_FUEL_TRANSFER_RATE * 0.1f; // Minimum 10% rate
        }

        rocket->startFuelTransferOut(transferRate);
        stateChanged = true;
    }
    else if (!fuelDecreasePressed && fuelDecreaseKeyPressed) {
        // Key just released - stop fuel transfer
        rocket->stopFuelTransfer();
        stateChanged = true;
    }

    // Update key states for next frame
    fuelIncreaseKeyPressed = fuelIncreasePressed;
    fuelDecreaseKeyPressed = fuelDecreasePressed;
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
            state.mass = rocket->getMass();  // NOW DYNAMIC!
            state.thrustLevel = rocket->getThrustLevel();
            state.isOnGround = false;

            // FUEL DATA TO STATE
            state.currentFuel = rocket->getCurrentFuel();
            state.maxFuel = rocket->getMaxFuel();
            state.isCollectingFuel = rocket->getIsCollectingFuel();
        }
        else if (!state.isRocket && vehicleManager->getCar()) {
            state.rotation = vehicleManager->getCar()->getRotation();
            state.mass = GameConstants::ROCKET_BASE_MASS;  // Cars use base mass
            state.thrustLevel = 0.0f;
            state.isOnGround = vehicleManager->getCar()->isOnGround();

            // Cars don't use fuel (for now)
            state.currentFuel = 0.0f;
            state.maxFuel = 0.0f;
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
                rocket->setFuel(state.currentFuel);  // This will also update mass
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

// FUEL SYSTEM GETTERS (updated for dynamic mass)
float Player::getCurrentFuel() const {
    if (vehicleManager && vehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
        return vehicleManager->getRocket()->getCurrentFuel();
    }
    return 0.0f; // Cars don't have fuel
}

float Player::getMaxFuel() const {
    if (vehicleManager && vehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
        return vehicleManager->getRocket()->getMaxFuel();
    }
    return 0.0f;
}

float Player::getFuelPercentage() const {
    if (vehicleManager && vehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
        return vehicleManager->getRocket()->getFuelPercentage();
    }
    return 0.0f;
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

// NEW MASS SYSTEM GETTERS
float Player::getCurrentMass() const {
    if (vehicleManager && vehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
        return vehicleManager->getRocket()->getMass();
    }
    return GameConstants::ROCKET_BASE_MASS; // Cars use base mass
}

float Player::getMaxMass() const {
    if (vehicleManager && vehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
        return vehicleManager->getRocket()->getMaxMass();
    }
    return GameConstants::ROCKET_BASE_MASS;
}

float Player::getMassPercentage() const {
    if (vehicleManager && vehicleManager->getActiveVehicleType() == VehicleType::ROCKET) {
        Rocket* rocket = vehicleManager->getRocket();
        return ((rocket->getMass() - rocket->getBaseMass()) / (rocket->getMaxMass() - rocket->getBaseMass())) * 100.0f;
    }
    return 0.0f;
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

void Player::handleSatelliteConversionInput(const sf::Event& event) {
    if (type != PlayerType::LOCAL || !satelliteManager) return;

    if (event.is<sf::Event::KeyPressed>()) {
        const auto* keyEvent = event.getIf<sf::Event::KeyPressed>();
        if (keyEvent) {
            // Use different keys based on player ID to avoid conflicts in split-screen
            sf::Keyboard::Key conversionKey = (playerID == 0) ? sf::Keyboard::Key::T : sf::Keyboard::Key::Y;

            if (keyEvent->code == conversionKey && !satelliteConversionKeyPressed) {
                satelliteConversionKeyPressed = true;
                convertRocketToSatellite();
            }
        }
    }

    if (event.is<sf::Event::KeyReleased>()) {
        const auto* keyEvent = event.getIf<sf::Event::KeyReleased>();
        if (keyEvent) {
            sf::Keyboard::Key conversionKey = (playerID == 0) ? sf::Keyboard::Key::T : sf::Keyboard::Key::Y;

            if (keyEvent->code == conversionKey) {
                satelliteConversionKeyPressed = false;
            }
        }
    }
}

bool Player::canConvertToSatellite() const {
    if (!satelliteManager || !vehicleManager) return false;
    if (vehicleManager->getActiveVehicleType() != VehicleType::ROCKET) return false;

    Rocket* rocket = vehicleManager->getRocket();
    return rocket && satelliteManager->canConvertRocketToSatellite(rocket);
}

void Player::convertRocketToSatellite() {
    if (isNetworkMultiplayerMode) {
        convertRocketToSatelliteNetwork();
        return;
    }

    // Original local conversion logic
    if (!canConvertToSatellite()) {
        std::cout << "Player " << playerName << " cannot convert rocket to satellite" << std::endl;
        return;
    }

    Rocket* rocket = vehicleManager->getRocket();
    if (!rocket) return;

    try {
        // Get optimal conversion configuration
        SatelliteConversionConfig config = satelliteManager->getOptimalConversionConfig(rocket);
        config.customName = playerName + "-SAT-" + std::to_string(satelliteManager->getSatelliteCount() + 1);

        // Create satellite from rocket
        int satelliteID = satelliteManager->createSatelliteFromRocket(rocket, config);

        if (satelliteID >= 0) {
            // Track ownership
            addOwnedSatellite(satelliteID);

            std::cout << "Player " << playerName << " converted rocket to satellite (ID: " << satelliteID << ")"
                << std::endl;

            // Create new rocket at nearest planet surface
            sf::Vector2f newRocketPos = findNearestPlanetSurface();
            respawnAtPosition(newRocketPos);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception during Player satellite conversion: " << e.what() << std::endl;
    }
}
sf::Vector2f Player::findNearestPlanetSurface() const {
    if (!vehicleManager) return spawnPosition;

    sf::Vector2f currentPos = vehicleManager->getActiveVehicle()->getPosition();
    Planet* nearestPlanet = nullptr;
    float minDistance = std::numeric_limits<float>::max();

    for (auto* planet : planets) {
        if (!planet) continue;
        sf::Vector2f direction = planet->getPosition() - currentPos;
        float dist = std::sqrt(direction.x * direction.x + direction.y * direction.y);
        if (dist < minDistance) {
            minDistance = dist;
            nearestPlanet = planet;
        }
    }

    if (nearestPlanet) {
        // Add player-specific offset to avoid spawn conflicts
        float playerOffset = playerID * 0.5f; // Slight angular offset per player
        sf::Vector2f direction = currentPos - nearestPlanet->getPosition();
        float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
        if (length > 0) {
            direction /= length; // normalize
        }

        // Apply small rotation based on player ID
        float cosOffset = std::cos(playerOffset);
        float sinOffset = std::sin(playerOffset);
        sf::Vector2f offsetDirection(
            direction.x * cosOffset - direction.y * sinOffset,
            direction.x * sinOffset + direction.y * cosOffset
        );

        return nearestPlanet->getPosition() + offsetDirection * (nearestPlanet->getRadius() + GameConstants::ROCKET_SIZE + 15.0f);
    }

    // Fallback to original spawn position with player offset
    return spawnPosition + sf::Vector2f(playerID * 50.0f, 0.0f);
}

// Network multiplayer methods
void Player::setNetworkManager(NetworkManager* netManager) {
    networkManager = netManager;
}

void Player::convertRocketToSatelliteNetwork() {
    if (!canConvertToSatellite() || !networkManager) {
        std::cout << "Player " << playerName << " cannot convert rocket to satellite in network mode" << std::endl;
        return;
    }

    Rocket* rocket = vehicleManager->getRocket();
    if (!rocket) return;

    try {
        // Prepare conversion info for network
        SatelliteCreationInfo creationInfo;
        creationInfo.satelliteID = satelliteManager->getSatelliteCount() + playerID * 100; // Avoid ID conflicts
        creationInfo.ownerPlayerID = playerID;
        creationInfo.position = rocket->getPosition();
        creationInfo.velocity = rocket->getVelocity();
        creationInfo.currentFuel = rocket->getCurrentFuel() * GameConstants::SATELLITE_CONVERSION_FUEL_RETENTION;
        creationInfo.maxFuel = rocket->getMaxFuel();
        creationInfo.name = playerName + "-SAT-" + std::to_string(creationInfo.satelliteID);

        // Send creation to network first
        if (networkManager->sendSatelliteCreated(creationInfo)) {
            // Create satellite locally
            int satelliteID = satelliteManager->createNetworkSatellite(creationInfo);

            if (satelliteID >= 0) {
                // Track ownership
                addOwnedSatellite(satelliteID);

                // Assign ownership in satellite manager
                satelliteManager->assignSatelliteOwner(satelliteID, playerID);

                std::cout << "Player " << playerName << " sent network satellite conversion (ID: " << satelliteID << ")" << std::endl;

                // Create new rocket at nearest planet surface
                sf::Vector2f newRocketPos = findNearestPlanetSurface();
                respawnAtPosition(newRocketPos);
            }
        }
        else {
            std::cout << "Failed to send satellite creation to network" << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception during network satellite conversion: " << e.what() << std::endl;
    }
}

void Player::handleNetworkSatelliteConversion(const SatelliteCreationInfo& creationInfo) {
    if (!satelliteManager) return;

    // Create satellite from network data (for remote players)
    int satelliteID = satelliteManager->createNetworkSatellite(creationInfo);

    if (satelliteID >= 0) {
        std::cout << "Player " << playerName << " received network satellite from Player "
            << creationInfo.ownerPlayerID << " (ID: " << satelliteID << ")" << std::endl;

        // If this was our conversion, handle rocket respawn
        if (creationInfo.ownerPlayerID == playerID) {
            addOwnedSatellite(satelliteID);
            // Note: respawn position should be handled by the conversion sender
        }
    }
}

void Player::syncWithNetwork(float deltaTime) {
    if (!networkManager || !isNetworkMultiplayerMode) return;

    // Send state updates at regular intervals
    if (timeSinceLastStateSent >= STATE_SEND_INTERVAL) {
        sendPlayerStateToNetwork();
        timeSinceLastStateSent = 0.0f;
    }

    // Send planet states if hosting
    if (networkManager->getRole() == NetworkRole::HOST && hasPendingPlanetUpdates) {
        sendPlanetStatesToNetwork();
        hasPendingPlanetUpdates = false;
    }

    // Handle incoming satellite creations
    while (networkManager->hasPendingSatelliteCreation()) {
        SatelliteCreationInfo creationInfo;
        if (networkManager->receiveSatelliteCreation(creationInfo)) {
            handleNetworkSatelliteConversion(creationInfo);
        }
    }

    // Handle incoming planet updates
    if (networkManager->hasPendingPlanetState()) {
        std::vector<PlanetState> planetUpdates = networkManager->getPendingPlanetUpdates();
        receivePlanetStatesFromNetwork(planetUpdates);
    }
}

void Player::sendPlayerStateToNetwork() {
    if (!networkManager) return;

    PlayerState state = createPlayerState();
    networkManager->sendPlayerState(state);
}

void Player::receivePlayerStateFromNetwork(const PlayerState& networkState) {
    if (type != PlayerType::REMOTE) return;

    applyPlayerState(networkState);
}

void Player::sendPlanetStatesToNetwork() {
    if (!networkManager || !satelliteManager) return;

    std::vector<PlanetState> planetStates = satelliteManager->getCurrentPlanetStates();
    networkManager->syncPlanetStates(planetStates);
}

void Player::receivePlanetStatesFromNetwork(const std::vector<PlanetState>& planetStates) {
    if (!satelliteManager) return;

    satelliteManager->updateFromNetworkPlanetStates(planetStates);
}

void Player::updateLocalPlanetsFromNetwork() {
    // Check if any planets have changed significantly
    for (Planet* planet : planets) {
        if (planet && planet->hasStateChanged()) {
            hasPendingPlanetUpdates = true;
            break;
        }
    }
}

void Player::addOwnedSatellite(int satelliteID) {
    if (std::find(ownedSatelliteIDs.begin(), ownedSatelliteIDs.end(), satelliteID) == ownedSatelliteIDs.end()) {
        ownedSatelliteIDs.push_back(satelliteID);
    }
}

void Player::removeOwnedSatellite(int satelliteID) {
    auto it = std::find(ownedSatelliteIDs.begin(), ownedSatelliteIDs.end(), satelliteID);
    if (it != ownedSatelliteIDs.end()) {
        ownedSatelliteIDs.erase(it);
    }
}

bool Player::ownsSatellite(int satelliteID) const {
    return std::find(ownedSatelliteIDs.begin(), ownedSatelliteIDs.end(), satelliteID) != ownedSatelliteIDs.end();
}

void Player::updateRocketTargetingForNetwork() {
    if (!satelliteManager || !vehicleManager) return;

    // Get our rocket for network targeting
    Rocket* ourRocket = getRocketForNetworkTargeting();
    if (ourRocket) {
        // Add our rocket to the satellite manager's network rocket list
        satelliteManager->addNetworkRocket(ourRocket, playerID);
    }
}

Rocket* Player::getRocketForNetworkTargeting() const {
    if (!vehicleManager || vehicleManager->getActiveVehicleType() != VehicleType::ROCKET) {
        return nullptr;
    }
    return vehicleManager->getRocket();
}



PlayerState Player::createPlayerState() const {
    PlayerState state;
    state.playerID = playerID;

    if (vehicleManager && vehicleManager->getRocket()) {
        Rocket* rocket = vehicleManager->getRocket();
        state.position = rocket->getPosition();
        state.velocity = rocket->getVelocity();
        state.rotation = rocket->getRotation();
        state.isRocket = true;
        state.isOnGround = false; // TODO: Add ground detection
        state.mass = rocket->getMass();
        state.thrustLevel = rocket->getThrustLevel();
        state.currentFuel = rocket->getCurrentFuel();
        state.maxFuel = rocket->getMaxFuel();
        state.isCollectingFuel = rocket->getIsCollectingFuel();
    }
    else {
        // Default values if no vehicle
        state.position = spawnPosition;
        state.velocity = sf::Vector2f(0, 0);
        state.rotation = 0.0f;
        state.isRocket = true;
        state.isOnGround = false;
        state.mass = GameConstants::ROCKET_BASE_MASS;
        state.thrustLevel = 0.0f;
        state.currentFuel = GameConstants::ROCKET_STARTING_FUEL;
        state.maxFuel = GameConstants::ROCKET_MAX_FUEL;
        state.isCollectingFuel = false;
    }

    // Satellite fields (not used for player state, but included for completeness)
    state.isSatellite = false;
    state.satelliteID = -1;
    state.orbitAccuracy = 100.0f;
    state.needsMaintenance = false;
    state.requestingSatelliteConversion = false;
    state.newSatelliteID = -1;
    state.newRocketSpawnPos = sf::Vector2f(0, 0);

    return state;
}

void Player::applyPlayerState(const PlayerState& state) {
    if (type != PlayerType::REMOTE) return;

    // Update vehicle state from network
    if (vehicleManager && vehicleManager->getRocket()) {
        Rocket* rocket = vehicleManager->getRocket();
        rocket->setPosition(state.position);
        rocket->setVelocity(state.velocity);
        rocket->setFuel(state.currentFuel);
        // Note: Other properties like rotation and thrust are visual only for remote players
    }
}

void Player::updateStateFromVehicle() {
    if (type != PlayerType::LOCAL || !vehicleManager) return;

    // Check if vehicle state has changed significantly
    static sf::Vector2f lastPosition(0, 0);
    static float lastFuel = 0;

    if (vehicleManager->getRocket()) {
        Rocket* rocket = vehicleManager->getRocket();
        sf::Vector2f currentPos = rocket->getPosition();
        float currentFuel = rocket->getCurrentFuel();

        // Check for significant changes
        float positionChange = std::sqrt((currentPos.x - lastPosition.x) * (currentPos.x - lastPosition.x) +
            (currentPos.y - lastPosition.y) * (currentPos.y - lastPosition.y));
        float fuelChange = std::abs(currentFuel - lastFuel);

        if (positionChange > 1.0f || fuelChange > 0.1f) {
            stateChanged = true;
            lastPosition = currentPos;
            lastFuel = currentFuel;
        }
    }
}