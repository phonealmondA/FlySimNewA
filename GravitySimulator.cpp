// GravitySimulator.cpp - Updated with Player Support and Fuel System
#include "GravitySimulator.h"
#include "VehicleManager.h"
#include "Player.h"
#include "Satellite.h"
#include "SatelliteManager.h"
#include <iostream>  
void GravitySimulator::addPlanet(Planet* planet)
{
    planets.push_back(planet);
}

void GravitySimulator::addRocket(Rocket* rocket)
{
    rockets.push_back(rocket);
}

void GravitySimulator::addVehicleManager(VehicleManager* manager)
{
    // Add to both for backward compatibility
    if (vehicleManager == nullptr) {
        vehicleManager = manager;  // Keep the first one for legacy code
    }
    vehicleManagers.push_back(manager);

    // Update satellite manager with new vehicle manager
    if (satelliteManager) {
        updateSatelliteManagerWithVehicleManagers();
    }
}

void GravitySimulator::addPlayer(Player* player)
{
    players.push_back(player);

    // Update satellite manager with new player's rocket
    if (satelliteManager && player && player->getVehicleManager()) {
        updateSatelliteManagerWithPlayers();
    }
}


void GravitySimulator::addSatellite(Satellite* satellite)
{
    satellites.push_back(satellite);
}

void GravitySimulator::addSatelliteManager(SatelliteManager* satManager)
{
    satelliteManager = satManager;
}

void GravitySimulator::removePlayer(int playerID)
{
    players.erase(
        std::remove_if(players.begin(), players.end(),
            [playerID](Player* p) { return p->getID() == playerID; }),
        players.end()
    );
}

void GravitySimulator::clearRockets()
{
    rockets.clear();
}

void GravitySimulator::addRocketGravityInteractions(float deltaTime)
{
    // Apply gravity between rockets
    for (size_t i = 0; i < rockets.size(); i++) {
        for (size_t j = i + 1; j < rockets.size(); j++) {
            Rocket* rocket1 = rockets[i];
            Rocket* rocket2 = rockets[j];

            sf::Vector2f direction = rocket2->getPosition() - rocket1->getPosition();
            float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

            // Minimum distance to prevent extreme forces when very close
            const float minDistance = GameConstants::TRAJECTORY_COLLISION_RADIUS;
            if (distance < minDistance) {
                distance = minDistance;
            }

            // Apply inverse square law for gravity using DYNAMIC MASS
            float forceMagnitude = G * rocket1->getMass() * rocket2->getMass() / (distance * distance);

            sf::Vector2f normalizedDir = normalize(direction);
            sf::Vector2f accel1 = normalizedDir * forceMagnitude / rocket1->getMass();
            sf::Vector2f accel2 = -normalizedDir * forceMagnitude / rocket2->getMass();

            rocket1->setVelocity(rocket1->getVelocity() + accel1 * deltaTime);
            rocket2->setVelocity(rocket2->getVelocity() + accel2 * deltaTime);
        }
    }
}

void GravitySimulator::applyGravityToPlayers(float deltaTime)
{
    // Apply gravity from planets to all players
    for (Player* player : players) {
        if (player && player->getVehicleManager()->getActiveVehicleType() == VehicleType::ROCKET) {
            Rocket* rocket = player->getVehicleManager()->getRocket();
            if (rocket) {
                for (auto planet : planets) {
                    sf::Vector2f direction = planet->getPosition() - rocket->getPosition();
                    float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

                    // Avoid division by zero and very small distances
                    if (distance > planet->getRadius() + GameConstants::TRAJECTORY_COLLISION_RADIUS) {
                        // Use rocket's DYNAMIC MASS for gravity calculation
                        float forceMagnitude = G * planet->getMass() * rocket->getMass() / (distance * distance);
                        sf::Vector2f acceleration = normalize(direction) * forceMagnitude / rocket->getMass();
                        sf::Vector2f velocityChange = acceleration * deltaTime;
                        rocket->setVelocity(rocket->getVelocity() + velocityChange);
                    }
                }
            }
        }
        // Car gravity is handled internally in Car::update
    }
}

void GravitySimulator::addPlayerGravityInteractions(float deltaTime)
{
    // Apply gravity between players (rocket-to-rocket)
    for (size_t i = 0; i < players.size(); i++) {
        for (size_t j = i + 1; j < players.size(); j++) {
            Player* player1 = players[i];
            Player* player2 = players[j];

            // Only apply rocket-to-rocket gravity
            if (player1->getVehicleManager()->getActiveVehicleType() == VehicleType::ROCKET &&
                player2->getVehicleManager()->getActiveVehicleType() == VehicleType::ROCKET) {

                Rocket* rocket1 = player1->getVehicleManager()->getRocket();
                Rocket* rocket2 = player2->getVehicleManager()->getRocket();

                sf::Vector2f direction = rocket2->getPosition() - rocket1->getPosition();
                float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

                // Minimum distance to prevent extreme forces when very close
                const float minDistance = GameConstants::TRAJECTORY_COLLISION_RADIUS;
                if (distance < minDistance) {
                    distance = minDistance;
                }

                // Apply inverse square law for gravity using DYNAMIC MASS
                float forceMagnitude = G * rocket1->getMass() * rocket2->getMass() / (distance * distance);

                sf::Vector2f normalizedDir = normalize(direction);
                sf::Vector2f accel1 = normalizedDir * forceMagnitude / rocket1->getMass();
                sf::Vector2f accel2 = -normalizedDir * forceMagnitude / rocket2->getMass();

                rocket1->setVelocity(rocket1->getVelocity() + accel1 * deltaTime);
                rocket2->setVelocity(rocket2->getVelocity() + accel2 * deltaTime);
            }
        }
    }
}

void GravitySimulator::processFuelCollectionForAllRockets(float deltaTime)
{
    // Process fuel collection for individual rockets (legacy single-player mode)
    for (auto rocket : rockets) {
        if (rocket) {
            // Set nearby planets for fuel collection
            rocket->setNearbyPlanets(planets);
            // Fuel collection is now handled inside rocket->update()
            // No additional processing needed here since manual transfer is handled by input
        }
    }
}

void GravitySimulator::processFuelCollectionForVehicleManagers(float deltaTime)
{
    // Process fuel collection for all vehicle managers (split-screen mode)
    for (VehicleManager* vm : vehicleManagers) {
        if (vm && vm->getActiveVehicleType() == VehicleType::ROCKET) {
            Rocket* rocket = vm->getRocket();
            if (rocket) {
                // Set nearby planets for fuel collection
                rocket->setNearbyPlanets(planets);
                // Fuel collection is now handled inside rocket->update()
                // No additional processing needed here since manual transfer is handled by input
            }
        }
    }
}

void GravitySimulator::processFuelCollectionForPlayers(float deltaTime)
{
    // Process fuel collection for all players (network multiplayer mode)
    for (Player* player : players) {
        if (player && player->getVehicleManager()->getActiveVehicleType() == VehicleType::ROCKET) {
            Rocket* rocket = player->getVehicleManager()->getRocket();
            if (rocket) {
                // Set nearby planets for fuel collection
                rocket->setNearbyPlanets(planets);
                // Fuel collection is now handled inside rocket->update() and Player::handleFuelTransferInput()
                // No additional processing needed here since manual transfer is handled by input
            }
        }
    }
}

void GravitySimulator::update(float deltaTime)
{
    // Apply gravity between planets if enabled
    if (simulatePlanetGravity) {
        for (size_t i = 0; i < planets.size(); i++) {
            for (size_t j = i + 1; j < planets.size(); j++) {
                Planet* planet1 = planets[i];
                Planet* planet2 = planets[j];

                // Skip the first planet (index 0) - it's pinned in place
                if (i == 0) {
                    // Only apply gravity from planet1 to planet2
                    sf::Vector2f direction = planet1->getPosition() - planet2->getPosition();
                    float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

                    if (distance > planet1->getRadius() + planet2->getRadius()) {
                        float forceMagnitude = G * planet1->getMass() * planet2->getMass() / (distance * distance);
                        sf::Vector2f normalizedDir = normalize(direction);
                        sf::Vector2f accel2 = normalizedDir * forceMagnitude / planet2->getMass();
                        planet2->setVelocity(planet2->getVelocity() + accel2 * deltaTime);
                    }
                }
                else {
                    // Regular gravity calculation between other planets
                    sf::Vector2f direction = planet2->getPosition() - planet1->getPosition();
                    float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

                    if (distance > planet1->getRadius() + planet2->getRadius()) {
                        float forceMagnitude = G * planet1->getMass() * planet2->getMass() / (distance * distance);
                        sf::Vector2f normalizedDir = normalize(direction);
                        sf::Vector2f accel1 = normalizedDir * forceMagnitude / planet1->getMass();
                        sf::Vector2f accel2 = -normalizedDir * forceMagnitude / planet2->getMass();
                        planet1->setVelocity(planet1->getVelocity() + accel1 * deltaTime);
                        planet2->setVelocity(planet2->getVelocity() + accel2 * deltaTime);
                    }
                }
            }
        }
    }

    // Apply gravity to satellites from planets
    if (satelliteManager) {
        auto satellites = satelliteManager->getAllSatellites();
        for (auto* satellite : satellites) {
            if (satellite && satellite->isOperational()) {
                satellite->setNearbyPlanets(planets);
                // Satellite will handle its own gravity in its update() method
                // but we can also apply satellite-to-satellite gravity here if needed

                // Apply satellite-to-satellite gravity
                for (auto* otherSat : satellites) {
                    if (otherSat && otherSat != satellite && otherSat->isOperational()) {
                        sf::Vector2f direction = otherSat->getPosition() - satellite->getPosition();
                        float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

                        const float minDistance = GameConstants::TRAJECTORY_COLLISION_RADIUS;
                        if (distance < minDistance) {
                            distance = minDistance;
                        }

                        float forceMagnitude = G * satellite->getMass() * otherSat->getMass() / (distance * distance);
                        sf::Vector2f normalizedDir = normalize(direction);
                        sf::Vector2f accel = normalizedDir * forceMagnitude / satellite->getMass();
                        satellite->setVelocity(satellite->getVelocity() + accel * deltaTime);
                    }
                }
            }
        }
    }

    // FUEL SYSTEM INTEGRATION: Process fuel collection for all game modes
    if (!players.empty()) {
        // Network multiplayer mode - handle players
        applyGravityToPlayers(deltaTime);
        addPlayerGravityInteractions(deltaTime);
        processFuelCollectionForPlayers(deltaTime);
    }
    else {
        // LOCAL MODES - DEBUG THE GRAVITY ISSUE
        static float debugTimer = 0.0f;
        debugTimer += deltaTime;

        if (debugTimer > 200.0f) { // Print debug info every 200 seconds
            debugTimer = 0.0f;

            std::cout << "\n=== GRAVITY DEBUG ===" << std::endl;
            std::cout << "Vehicle Managers count: " << vehicleManagers.size() << std::endl;
            std::cout << "Planets count: " << planets.size() << std::endl;

            // Print all planet info
            for (size_t i = 0; i < planets.size(); i++) {
                auto planet = planets[i];
                if (planet) {
                    std::cout << "Planet " << i << ": pos(" << planet->getPosition().x
                        << ", " << planet->getPosition().y << ") mass=" << planet->getMass()
                        << " radius=" << planet->getRadius() << std::endl;
                }
            }
        }

        // Apply gravity to ALL vehicle managers (split-screen and single-player)
        for (VehicleManager* vm : vehicleManagers) {
            if (vm && vm->getActiveVehicleType() == VehicleType::ROCKET) {
                Rocket* rocket = vm->getRocket();
                if (rocket) {
                    sf::Vector2f rocketPos = rocket->getPosition();

                    // DEBUG: Print rocket position occasionally
                    if (debugTimer == 0.0f) { // Only when we just printed planet info
                        std::cout << "Rocket pos: (" << rocketPos.x << ", " << rocketPos.y << ")" << std::endl;
                    }

                    for (size_t planetIndex = 0; planetIndex < planets.size(); planetIndex++) {
                        auto planet = planets[planetIndex];
                        sf::Vector2f direction = planet->getPosition() - rocketPos;
                        float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

                        float collisionRadius = planet->getRadius() + GameConstants::TRAJECTORY_COLLISION_RADIUS;
                        bool gravityEnabled = distance > collisionRadius;

                        // DEBUG: Print gravity info for each planet occasionally
                        if (debugTimer == 0.0f && planetIndex < 5) { // First 5 planets only
                            std::cout << "Planet " << planetIndex << " dist=" << distance
                                << " collisionRadius=" << collisionRadius
                                << " gravityEnabled=" << (gravityEnabled ? "YES" : "NO") << std::endl;

                            if (gravityEnabled) {
                                float forceMagnitude = G * planet->getMass() * rocket->getMass() / (distance * distance);
                                std::cout << "  -> Force magnitude: " << forceMagnitude << std::endl;
                            }
                        }

                        // Avoid division by zero and very small distances
                        if (gravityEnabled) {
                            // Use rocket's DYNAMIC MASS for gravity calculation
                            float forceMagnitude = G * planet->getMass() * rocket->getMass() / (distance * distance);
                            sf::Vector2f acceleration = normalize(direction) * forceMagnitude / rocket->getMass();
                            sf::Vector2f velocityChange = acceleration * deltaTime;
                            rocket->setVelocity(rocket->getVelocity() + velocityChange);
                        }
                    }
                }
            }
            // Car gravity is handled internally in Car::update
        }

        // Process fuel collection for vehicle managers
        processFuelCollectionForVehicleManagers(deltaTime);

        // Legacy code for handling individual rockets (still needed for single-player mode)
        for (auto rocket : rockets) {
            for (auto planet : planets) {
                sf::Vector2f direction = planet->getPosition() - rocket->getPosition();
                float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

                // Avoid division by zero and very small distances
                if (distance > planet->getRadius() + GameConstants::TRAJECTORY_COLLISION_RADIUS) {
                    float forceMagnitude = G * planet->getMass() * rocket->getMass() / (distance * distance);
                    sf::Vector2f acceleration = normalize(direction) * forceMagnitude / rocket->getMass();
                    sf::Vector2f velocityChange = acceleration * deltaTime;
                    rocket->setVelocity(rocket->getVelocity() + velocityChange);
                }
            }
        }

        // Process fuel collection for individual rockets
        processFuelCollectionForAllRockets(deltaTime);

        // Add rocket-to-rocket gravity interactions using DYNAMIC MASS
        addRocketGravityInteractions(deltaTime);
    }

    // FUEL SYSTEM: The fuel consumption and collection is handled automatically 
    // in each Rocket's update() method and Player's handleFuelTransferInput() method
    // Planet mass reduction from fuel collection is handled in Rocket's manual transfer methods

    // SATELLITE SYSTEM: Update satellite manager with current rockets
    if (satelliteManager) {
        if (!players.empty()) {
            updateSatelliteManagerWithPlayers();
        }
        else {
            updateSatelliteManagerWithVehicleManagers();
        }
    }
}
void GravitySimulator::updateSatelliteManagerWithPlayers() {
    if (!satelliteManager) return;

    std::vector<Rocket*> activeRockets;

    for (Player* player : players) {
        if (player && player->getVehicleManager() &&
            player->getVehicleManager()->getActiveVehicleType() == VehicleType::ROCKET) {
            Rocket* rocket = player->getVehicleManager()->getRocket();
            if (rocket) {
                activeRockets.push_back(rocket);
            }
        }
    }

    satelliteManager->setNearbyRockets(activeRockets);
}

void GravitySimulator::updateSatelliteManagerWithVehicleManagers() {
    if (!satelliteManager) return;

    std::vector<Rocket*> activeRockets;

    for (VehicleManager* vm : vehicleManagers) {
        if (vm && vm->getActiveVehicleType() == VehicleType::ROCKET) {
            Rocket* rocket = vm->getRocket();
            if (rocket) {
                activeRockets.push_back(rocket);
            }
        }
    }

    satelliteManager->setNearbyRockets(activeRockets);
}