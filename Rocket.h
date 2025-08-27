#pragma once
#ifndef ROCKET_H
#define ROCKET_H

#include "GameObject.h"
#include "GameConstants.h"
#include "RocketPart.h"
#include "Engine.h"
#include <vector>
#include <memory>

// Forward declarations
class Planet;

class Rocket : public GameObject {
private:
    sf::ConvexShape body;
    std::vector<std::unique_ptr<RocketPart>> parts;
    float rotation;
    float angularVelocity;
    float thrustLevel; // Current thrust level (0.0 to 1.0)
    std::vector<Planet*> nearbyPlanets;

    // DYNAMIC MASS SYSTEM
    float mass; // Now dynamic - changes with fuel
    float baseMass; // Empty rocket mass (constant)
    float maxMass; // Maximum allowed mass

    // FUEL SYSTEM PROPERTIES
    float currentFuel;  // Current fuel amount
    float maxFuel;      // Maximum fuel capacity
    bool isCollectingFuel;  // Flag to track if currently collecting fuel
    Planet* fuelSourcePlanet;  // Which planet we're collecting fuel from
    bool isCurrentlyThrusting;  // Track if thrust is actually being applied this frame

    // MANUAL FUEL TRANSFER SYSTEM
    bool isTransferringFuelIn;   // Taking fuel from planet
    bool isTransferringFuelOut;  // Giving fuel to planet
    float fuelTransferRate;      // Current transfer rate

    bool checkCollision(const Planet& planet);

    // FUEL SYSTEM METHODS
    void consumeFuel(float deltaTime);  // Consume fuel based on thrust level
    void updateMassFromFuel();  // Update rocket mass based on current fuel
    void preserveMomentumDuringMassChange(float oldMass, float newMass);  // Maintain momentum when mass changes
    float calculateFuelConsumption() const;  // Calculate current fuel consumption rate

    // MANUAL FUEL TRANSFER METHODS
    void processManualFuelTransfer(float deltaTime);  // Handle manual fuel operations
    bool canTransferFuelFromPlanet(Planet* planet, float amount) const;  // Check if transfer is possible
    bool canTransferFuelToPlanet(Planet* planet, float amount) const;   // Check if transfer is possible

public:
    Rocket(sf::Vector2f pos, sf::Vector2f vel, sf::Color col = sf::Color::White, float baseM = GameConstants::ROCKET_BASE_MASS);

    void addPart(std::unique_ptr<RocketPart> part);
    void applyThrust(float amount);
    void rotate(float amount);
    void setThrustLevel(float level); // Set thrust level between 0.0 and 1.0
    bool isColliding(const Planet& planet);
    void setNearbyPlanets(const std::vector<Planet*>& planets) { nearbyPlanets = planets; }
    void setPosition(sf::Vector2f pos) { position = pos; }
    Rocket* mergeWith(Rocket* other);

    // MASS SYSTEM GETTERS
    float getMass() const { return mass; }
    float getBaseMass() const { return baseMass; }
    float getMaxMass() const { return maxMass; }
    float getMassCapacityRemaining() const { return maxMass - mass; }

    // FUEL SYSTEM GETTERS
    float getCurrentFuel() const { return currentFuel; }
    float getMaxFuel() const { return maxFuel; }
    float getFuelPercentage() const { return maxFuel > 0 ? (currentFuel / maxFuel) * 100.0f : 0.0f; }
    bool canThrust() const { return currentFuel > 0.0f; }  // Check if rocket has fuel to thrust
    bool getIsCollectingFuel() const { return isCollectingFuel; }
    Planet* getFuelSourcePlanet() const { return fuelSourcePlanet; }

    // MANUAL FUEL TRANSFER CONTROL
    void startFuelTransferIn(float transferRate);   // Begin taking fuel from planet
    void startFuelTransferOut(float transferRate);  // Begin giving fuel to planet
    void stopFuelTransfer();  // Stop all fuel transfer operations
    bool isTransferringFuel() const { return isTransferringFuelIn || isTransferringFuelOut; }
    float getCurrentTransferRate() const { return fuelTransferRate; }

    // FUEL SYSTEM SETTERS
    void setFuel(float fuel);  // Now handles mass updates
    void addFuel(float fuel);  // Now handles mass updates

    // AUTOMATIC FUEL COLLECTION (for future satellites)
    void collectFuelFromPlanetsAuto(float deltaTime);  // Moved from update() - for satellites

    void update(float deltaTime) override;
    void draw(sf::RenderWindow& window) override;
    void drawWithConstantSize(sf::RenderWindow& window, float zoomLevel);

    // Draw velocity vector line
    void drawVelocityVector(sf::RenderWindow& window, float scale = GameConstants::VELOCITY_VECTOR_SCALE);

    // New method to draw gravity force vectors
    void drawGravityForceVectors(sf::RenderWindow& window, const std::vector<Planet*>& planets, float scale = 1.0f);

    void drawTrajectory(sf::RenderWindow& window, const std::vector<Planet*>& planets,
        float timeStep = 0.5f, int steps = 200, bool detectSelfIntersection = false);
    float getThrustLevel() const { return thrustLevel; }
    const std::vector<std::unique_ptr<RocketPart>>& getParts() const { return parts; }
    float getRotation() const { return rotation; }
};

#endif // ROCKET_H