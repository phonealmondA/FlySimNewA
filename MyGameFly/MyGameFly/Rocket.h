#pragma once
#include "GameObject.h"
#include "GameConstants.h"
#include "RocketPart.h"
#include "Engine.h"
#include "Planet.h"
#include <vector>
#include <memory>

class Rocket : public GameObject {
private:
    sf::ConvexShape body;
    std::vector<std::unique_ptr<RocketPart>> parts;
    float rotation;
    float angularVelocity;
    float thrustLevel; // Current thrust level (0.0 to 1.0)
    std::vector<Planet*> nearbyPlanets;
    float mass; // Added mass property for physics calculations

    // FUEL SYSTEM PROPERTIES
    float currentFuel;  // Current fuel amount
    float maxFuel;      // Maximum fuel capacity
    bool isCollectingFuel;  // Flag to track if currently collecting fuel
    Planet* fuelSourcePlanet;  // Which planet we're collecting fuel from

    bool checkCollision(const Planet& planet);

    // FUEL SYSTEM METHODS
    void consumeFuel(float deltaTime);  // Consume fuel based on thrust level
    void collectFuelFromPlanets(float deltaTime);  // Collect fuel from nearby planets
    float calculateFuelConsumption() const;  // Calculate current fuel consumption rate

public:
    Rocket(sf::Vector2f pos, sf::Vector2f vel, sf::Color col = sf::Color::White, float m = 1.0f);

    void addPart(std::unique_ptr<RocketPart> part);
    void applyThrust(float amount);
    void rotate(float amount);
    void setThrustLevel(float level); // Set thrust level between 0.0 and 1.0
    bool isColliding(const Planet& planet);
    void setNearbyPlanets(const std::vector<Planet*>& planets) { nearbyPlanets = planets; }
    void setPosition(sf::Vector2f pos) { position = pos; }
    Rocket* mergeWith(Rocket* other);

    // Add getter for mass
    float getMass() const { return mass; }

    // FUEL SYSTEM GETTERS
    float getCurrentFuel() const { return currentFuel; }
    float getMaxFuel() const { return maxFuel; }
    float getFuelPercentage() const { return maxFuel > 0 ? (currentFuel / maxFuel) * 100.0f : 0.0f; }
    bool canThrust() const { return currentFuel > 0.0f; }  // Check if rocket has fuel to thrust
    bool getIsCollectingFuel() const { return isCollectingFuel; }
    Planet* getFuelSourcePlanet() const { return fuelSourcePlanet; }

    // FUEL SYSTEM SETTERS
    void setFuel(float fuel) { currentFuel = std::max(0.0f, std::min(fuel, maxFuel)); }
    void addFuel(float fuel) { setFuel(currentFuel + fuel); }

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