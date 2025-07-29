// VehicleManager.cpp
#include "VehicleManager.h"
#include "GameConstants.h"
#include "VectorHelper.h"

VehicleManager::VehicleManager(sf::Vector2f initialPos, const std::vector<Planet*>& planetList)
    : activeVehicle(VehicleType::ROCKET), planets(planetList)
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
        }
    }
}

void VehicleManager::update(float deltaTime) {
    if (activeVehicle == VehicleType::ROCKET) {
        rocket->setNearbyPlanets(planets);
        rocket->update(deltaTime);
    }
    else {
        car->checkGrounding(planets);
        car->update(deltaTime);
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
        //rocket->setThrustLevel(1.0f); // Set thrust level to max
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