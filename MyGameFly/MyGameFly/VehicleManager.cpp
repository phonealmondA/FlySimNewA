// VehicleManager.cpp
#include "VehicleManager.h"
#include "GameConstants.h"
#include "VectorHelper.h"
#include "SatelliteManager.h"
#include <limits>
#include <iostream>  // For std::cout, std::cerr
#include <sstream>   // For std::stringstream

VehicleManager::VehicleManager(sf::Vector2f initialPos, const std::vector<Planet*>& planetList, SatelliteManager* satManager)
    : activeVehicle(VehicleType::ROCKET), planets(planetList), satelliteManager(satManager),
    isNetworkMultiplayerMode(false), playerID(-1), networkManager(nullptr)
{
    // Create initial rocket
    rocket = std::make_unique<Rocket>(initialPos, sf::Vector2f(0, 0));
    rocket->setNearbyPlanets(planets);

    // Initialize car (will be inactive at start)
    car = std::make_unique<Car>(initialPos, sf::Vector2f(0, 0));
}

void VehicleManager::switchVehicle() {
    if (activeVehicle == VehicleType::ROCKET) {
        // Check if rocket is close to a planet surface
        bool canTransform = false;
        for (const auto& planet : planets) {
            float dist = distance(rocket->getPosition(), planet->getPosition());
            if (dist <= planet->getRadius() + GameConstants::TRANSFORM_DISTANCE) {
                canTransform = true;
                break;
            }
        }

        if (canTransform) {
            // Remove rocket from network targeting before switching
            if (isNetworkMultiplayerMode) {
                removeFromNetworkTargeting();
            }

            // Transfer rocket state to car
            car->initializeFromRocket(rocket.get());
            car->checkGrounding(planets);
            activeVehicle = VehicleType::CAR;
        }
    }
    else {
        // Only allow switching back to rocket if car is on ground
        if (car->isOnGround()) {
            // Transfer car state to rocket
            rocket->setPosition(car->getPosition());
            rocket->setVelocity(sf::Vector2f(0, 0)); // Start with zero velocity
            activeVehicle = VehicleType::ROCKET;

            // Add rocket back to network targeting
            if (isNetworkMultiplayerMode) {
                updateNetworkRocketTargeting();
            }
        }
    }
}
void VehicleManager::update(float deltaTime) {
    if (activeVehicle == VehicleType::ROCKET) {
        rocket->setNearbyPlanets(planets);
        rocket->update(deltaTime);

        // Update satellite manager with current rocket
        updateSatelliteManager();

        // Handle network satellite synchronization
        if (isNetworkMultiplayerMode) {
            syncNetworkSatellites();
            updateNetworkRocketTargeting();
        }
    }
    else {
        car->checkGrounding(planets);
        car->update(deltaTime);

        // Remove rocket from network targeting when using car
        if (isNetworkMultiplayerMode) {
            removeFromNetworkTargeting();
        }
    }
}

void VehicleManager::draw(sf::RenderWindow& window) {
    if (activeVehicle == VehicleType::ROCKET) {
        rocket->draw(window);
    }
    else {
        car->draw(window);
    }
}

void VehicleManager::drawWithConstantSize(sf::RenderWindow& window, float zoomLevel) {
    if (activeVehicle == VehicleType::ROCKET) {
        rocket->drawWithConstantSize(window, zoomLevel);
    }
    else {
        car->drawWithConstantSize(window, zoomLevel);
    }
}

void VehicleManager::applyThrust(float amount) {
    if (activeVehicle == VehicleType::ROCKET) {
        rocket->applyThrust(amount);
    }
    else {
        car->accelerate(amount);
    }
}

void VehicleManager::rotate(float amount) {
    if (activeVehicle == VehicleType::ROCKET) {
        rocket->rotate(amount);
    }
    else {
        car->rotate(amount);
    }
}

void VehicleManager::drawVelocityVector(sf::RenderWindow& window, float scale) {
    if (activeVehicle == VehicleType::ROCKET) {
        rocket->drawVelocityVector(window, scale);
    }
    // Car doesn't have a velocity vector display
}

GameObject* VehicleManager::getActiveVehicle() {
    if (activeVehicle == VehicleType::ROCKET) {
        return rocket.get();
    }
    else {
        return car.get();
    }
}

Rocket* VehicleManager::getCurrentRocket() const {
    if (activeVehicle == VehicleType::ROCKET) {
        return rocket.get();
    }
    return nullptr;
}

bool VehicleManager::canConvertToSatellite() const {
    if (!satelliteManager || activeVehicle != VehicleType::ROCKET) return false;

    return rocket && satelliteManager->canConvertRocketToSatellite(rocket.get());
}

int VehicleManager::convertRocketToSatellite() {
    if (isNetworkMultiplayerMode) {
        return convertRocketToSatelliteNetwork();
    }

    // Original local conversion logic
    if (!canConvertToSatellite()) return -1;

    try {
        // Get optimal conversion configuration
        SatelliteConversionConfig config = satelliteManager->getOptimalConversionConfig(rocket.get());

        // Create satellite from rocket
        int satelliteID = satelliteManager->createSatelliteFromRocket(rocket.get(), config);

        if (satelliteID >= 0) {
            // Create new rocket at nearest planet surface
            sf::Vector2f newRocketPos = findNearestPlanetSurface();
            rocket = std::make_unique<Rocket>(newRocketPos, sf::Vector2f(0, 0));
            rocket->setNearbyPlanets(planets);

            // Update satellite manager
            updateSatelliteManager();

            std::cout << "VehicleManager converted rocket to satellite (ID: " << satelliteID
                << "), new rocket at (" << newRocketPos.x << ", " << newRocketPos.y << ")" << std::endl;
        }

        return satelliteID;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception during VehicleManager satellite conversion: " << e.what() << std::endl;
        return -1;
    }
}
sf::Vector2f VehicleManager::findNearestPlanetSurface() const {
    if (!rocket) return sf::Vector2f(GameConstants::MAIN_PLANET_X, GameConstants::MAIN_PLANET_Y - 150.0f);

    sf::Vector2f currentPos = rocket->getPosition();
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
        sf::Vector2f direction = currentPos - nearestPlanet->getPosition();
        float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
        if (length > 0) {
            direction /= length; // normalize
        }
        return nearestPlanet->getPosition() + direction * (nearestPlanet->getRadius() + GameConstants::ROCKET_SIZE + 10.0f);
    }

    // Fallback to default position
    return sf::Vector2f(GameConstants::MAIN_PLANET_X, GameConstants::MAIN_PLANET_Y - 150.0f);
}

void VehicleManager::updateSatelliteManager() {
    if (!satelliteManager) return;

    if (isNetworkMultiplayerMode) {
        // In network mode, add this rocket to the global network rocket list
        updateSatelliteManagerNetwork();
    }
    else {
        // Local mode - just set our rocket
        if (activeVehicle == VehicleType::ROCKET && rocket) {
            satelliteManager->setNearbyRockets({ rocket.get() });
        }
    }
}
// Network multiplayer support methods
void VehicleManager::setNetworkMode(bool enabled, int playerIdx, NetworkManager* netManager) {
    isNetworkMultiplayerMode = enabled;
    playerID = playerIdx;
    networkManager = netManager;

    // Update satellite manager with network mode
    if (satelliteManager && enabled) {
        satelliteManager->enableNetworkMode(true);
        satelliteManager->setNetworkManager(netManager);
    }

    std::cout << "VehicleManager: Set network mode " << (enabled ? "ON" : "OFF")
        << " for player " << playerID << std::endl;
}

int VehicleManager::convertRocketToSatelliteNetwork() {
    if (!canConvertToSatellite() || !networkManager || !satelliteManager) {
        std::cout << "VehicleManager: Cannot convert rocket to satellite in network mode" << std::endl;
        return -1;
    }

    try {
        // Prepare satellite creation info for network
        SatelliteCreationInfo creationInfo;
        creationInfo.satelliteID = satelliteManager->getSatelliteCount() + playerID * 100; // Avoid ID conflicts
        creationInfo.ownerPlayerID = playerID;
        creationInfo.position = rocket->getPosition();
        creationInfo.velocity = rocket->getVelocity();
        creationInfo.currentFuel = rocket->getCurrentFuel() * GameConstants::SATELLITE_CONVERSION_FUEL_RETENTION;
        creationInfo.maxFuel = rocket->getMaxFuel();

        // Generate name
        std::stringstream ss;
        ss << "P" << (playerID + 1) << "-SAT-" << creationInfo.satelliteID;
        creationInfo.name = ss.str();

        // Send creation to network first
        if (networkManager->sendSatelliteCreated(creationInfo)) {
            // Create satellite locally
            int satelliteID = satelliteManager->createNetworkSatellite(creationInfo);

            if (satelliteID >= 0) {
                // Assign ownership in satellite manager
                satelliteManager->assignSatelliteOwner(satelliteID, playerID);

                std::cout << "VehicleManager: Sent network satellite conversion (ID: " << satelliteID << ")" << std::endl;

                // Create new rocket at nearest planet surface
                sf::Vector2f newRocketPos = findNearestPlanetSurface();
                rocket = std::make_unique<Rocket>(newRocketPos, sf::Vector2f(0, 0));
                rocket->setNearbyPlanets(planets);

                // Update satellite manager with new rocket
                updateSatelliteManagerNetwork();

                return satelliteID;
            }
        }
        else {
            std::cout << "VehicleManager: Failed to send satellite creation to network" << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception during VehicleManager network satellite conversion: " << e.what() << std::endl;
    }

    return -1;
}

void VehicleManager::updateSatelliteManagerNetwork() {
    if (!satelliteManager || !isNetworkMultiplayerMode) return;

    // Add our rocket to the network rocket list for satellite targeting
    if (activeVehicle == VehicleType::ROCKET && rocket) {
        satelliteManager->addNetworkRocket(rocket.get(), playerID);
    }
}

void VehicleManager::removeFromNetworkTargeting() {
    if (!satelliteManager || !isNetworkMultiplayerMode) return;

    // Remove our rocket from network targeting when switching vehicles or disconnecting
    if (rocket) {
        satelliteManager->removeNetworkRocket(rocket.get());
    }
}

void VehicleManager::handleNetworkSatelliteCreation(const SatelliteCreationInfo& creationInfo) {
    if (!satelliteManager) return;

    // Create satellite from network data (for remote players)
    int satelliteID = satelliteManager->createNetworkSatellite(creationInfo);

    if (satelliteID >= 0) {
        std::cout << "VehicleManager: Received network satellite from Player "
            << creationInfo.ownerPlayerID << " (ID: " << satelliteID << ")" << std::endl;
    }
}

Rocket* VehicleManager::getRocketForNetworkTargeting() const {
    if (activeVehicle == VehicleType::ROCKET) {
        return rocket.get();
    }
    return nullptr;
}

void VehicleManager::syncNetworkSatellites() {
    if (!satelliteManager || !networkManager || !isNetworkMultiplayerMode) return;

    // Handle incoming satellite creations
    while (networkManager->hasPendingSatelliteCreation()) {
        SatelliteCreationInfo creationInfo;
        if (networkManager->receiveSatelliteCreation(creationInfo)) {
            // Only handle if it's not our own satellite
            if (creationInfo.ownerPlayerID != playerID) {
                handleNetworkSatelliteCreation(creationInfo);
            }
        }
    }
}

void VehicleManager::setAllNetworkRockets(const std::vector<Rocket*>& allNetworkRockets) {
    if (!satelliteManager) return;

    // Update satellite manager with all rockets from all players
    satelliteManager->setAllNetworkRockets(allNetworkRockets);

    std::cout << "VehicleManager: Updated satellite manager with " << allNetworkRockets.size()
        << " network rockets for targeting" << std::endl;
}

bool VehicleManager::isInNetworkMode() const {
    return isNetworkMultiplayerMode;
}

int VehicleManager::getPlayerID() const {
    return playerID;
}

void VehicleManager::updateNetworkRocketTargeting() {
    if (!isNetworkMultiplayerMode || !satelliteManager) return;

    // Ensure our rocket is available for satellite fuel targeting
    updateSatelliteManagerNetwork();
}
void VehicleManager::restoreFromSaveState(const PlayerState& saveState) {
    // Set the correct vehicle type based on save state
    if (saveState.isRocket) {
        activeVehicle = VehicleType::ROCKET;

        // Apply rocket state
        rocket->setPosition(saveState.position);
        rocket->setVelocity(saveState.velocity);
        rocket->setFuel(saveState.currentFuel);

        // Set rocket rotation and other properties
        if (rocket) {
            // Apply rotation if the rocket supports it
            float currentRotation = rocket->getRotation();
            float rotationDiff = saveState.rotation - currentRotation;
            rocket->rotate(rotationDiff);

            // Apply fuel collection state
            if (saveState.isCollectingFuel) {
                // The rocket will automatically detect nearby planets for fuel collection
                rocket->collectFuelFromPlanetsAuto(0.0f);
            }
        }

        std::cout << "VehicleManager: Restored rocket state (Fuel: " << saveState.currentFuel
            << ", Position: " << saveState.position.x << ", " << saveState.position.y << ")" << std::endl;
    }
    else {
        activeVehicle = VehicleType::CAR;

        // Apply car state
        car->setPosition(saveState.position);
        car->setVelocity(saveState.velocity);

        // Check grounding after position is set
        car->checkGrounding(planets);

        std::cout << "VehicleManager: Restored car state (Position: "
            << saveState.position.x << ", " << saveState.position.y << ")" << std::endl;
    }
}

void VehicleManager::applyPlayerState(const PlayerState& state) {
    // This method is for real-time state updates (like from network)
    // Similar to restoreFromSaveState but designed for frequent updates

    if (state.isRocket && activeVehicle == VehicleType::ROCKET) {
        rocket->setPosition(state.position);
        rocket->setVelocity(state.velocity);

        // Only update fuel if it's significantly different to avoid constant updates
        if (std::abs(rocket->getCurrentFuel() - state.currentFuel) > 1.0f) {
            rocket->setFuel(state.currentFuel);
        }
    }
    else if (!state.isRocket && activeVehicle == VehicleType::CAR) {
        car->setPosition(state.position);
        car->setVelocity(state.velocity);
        car->checkGrounding(planets);
    }
    else {
        // Vehicle type mismatch - need to switch vehicle type
        if (state.isRocket && activeVehicle == VehicleType::CAR) {
            // Switch to rocket
            activeVehicle = VehicleType::ROCKET;
            rocket->setPosition(state.position);
            rocket->setVelocity(state.velocity);
            rocket->setFuel(state.currentFuel);
        }
        else if (!state.isRocket && activeVehicle == VehicleType::ROCKET) {
            // Switch to car
            activeVehicle = VehicleType::CAR;
            car->setPosition(state.position);
            car->setVelocity(state.velocity);
            car->checkGrounding(planets);
        }
    }
}