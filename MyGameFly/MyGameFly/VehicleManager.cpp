// VehicleManager.cpp
#include "VehicleManager.h"
#include "GameConstants.h"
#include "VectorHelper.h"
#include "SatelliteManager.h"
#include <limits>

VehicleManager::VehicleManager(sf::Vector2f initialPos, const std::vector<Planet*>& planetList, SatelliteManager* satManager)
    : activeVehicle(VehicleType::ROCKET), planets(planetList), satelliteManager(satManager)
{
    // Create initial rocket
    rocket = std::make_unique<Rocket>(initialPos, sf::Vector2f(0, 0));
    rocket->setNearbyPlanets(planets);

    // Initialize car (will be inactive at start)
}

void VehicleManager::switchVehicle() {
    if (activeVehicle == VehicleType::ROCKET) {
        // Check if rocket is close to a planet surface
        bool canTransform = false;
    }
}

void VehicleManager::update(float deltaTime) {
    if (activeVehicle == VehicleType::ROCKET) {
        rocket->setNearbyPlanets(planets);
        rocket->update(deltaTime);

        // Update satellite manager with current rocket
        updateSatelliteManager();
    }
    else {
    }
}


void VehicleManager::draw(sf::RenderWindow& window) {
    if (activeVehicle == VehicleType::ROCKET) {
        rocket->draw(window);
    }
    else {
    }
}

void VehicleManager::drawWithConstantSize(sf::RenderWindow& window, float zoomLevel) {
    if (activeVehicle == VehicleType::ROCKET) {
        rocket->drawWithConstantSize(window, zoomLevel);
    }
    else {
    }
}

void VehicleManager::applyThrust(float amount) {
    if (activeVehicle == VehicleType::ROCKET) {
        rocket->applyThrust(amount);
    }
    else {
    }
}

void VehicleManager::rotate(float amount) {
    if (activeVehicle == VehicleType::ROCKET) {
        rocket->rotate(amount);
    }
    else {
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

            //std::cout << "VehicleManager converted rocket to satellite (ID: " << satelliteID
                //<< "), new rocket at (" << newRocketPos.x << ", " << newRocketPos.y << ")" << std::endl;
        }

        return satelliteID;
    }
    catch (const std::exception& e) {
        //std::cerr << "Exception during VehicleManager satellite conversion: " << e.what() << std::endl;
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

    // Update satellite manager with current rocket reference
    if (activeVehicle == VehicleType::ROCKET && rocket) {
        satelliteManager->setNearbyRockets({ rocket.get() });
    }
}

void VehicleManager::drawGravityForceVectors(sf::RenderWindow& window, float scale) {
    if (activeVehicle == VehicleType::ROCKET) {
        rocket->drawGravityForceVectors(window, planets, scale);
    }
}

void VehicleManager::drawTrajectory(sf::RenderWindow& window, float timeStep, int steps, bool detectSelfIntersection) {
    if (activeVehicle == VehicleType::ROCKET) {
        rocket->drawTrajectory(window, planets, timeStep, steps, detectSelfIntersection);
    }
}