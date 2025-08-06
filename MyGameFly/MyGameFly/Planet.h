#pragma once
#include "GameObject.h"
#include <vector>
#include <memory>

// Planet configuration structure for dynamic planet creation
struct PlanetConfig {
    float massRatio;                    // Mass as ratio of main planet
    float orbitDistanceMultiplier;      // Distance multiplier from base orbit
    float angleOffset;                  // Starting angle offset (radians)
    sf::Color color;

    PlanetConfig(float mass = 0.06f, float dist = 1.0f,
        float angle = 0.0f, sf::Color col = sf::Color::Green)
        : massRatio(mass), orbitDistanceMultiplier(dist),
        angleOffset(angle), color(col) {
    }
};

class Planet : public GameObject {
private:
    sf::CircleShape shape;
    float mass;
    float radius;

public:
    Planet(sf::Vector2f pos, float radius, float mass, sf::Color color = sf::Color::Blue);

    void update(float deltaTime) override;
    void draw(sf::RenderWindow& window) override;

    float getMass() const;
    float getRadius() const;

    // New methods for dynamic radius
    void setMass(float newMass);
    void updateRadiusFromMass();

    // Draw velocity vector for the planet
    void drawVelocityVector(sf::RenderWindow& window, float scale = 1.0f);

    // Draw predicted orbit path
    void drawOrbitPath(sf::RenderWindow& window, const std::vector<Planet*>& planets, float timeStep = 0.5f, int steps = 200);

    // FUEL COLLECTION METHODS
    void drawFuelCollectionRing(sf::RenderWindow& window, bool isActivelyCollecting = false);
    bool canCollectFuel() const;  // Check if planet has enough mass for fuel collection
    float getFuelCollectionRange() const;  // Get the fuel collection range for this planet

    // STATIC METHODS FOR DYNAMIC PLANET CREATION
    // Static method to create an orbiting planet
    static std::unique_ptr<Planet> createOrbitalPlanet(
        const Planet* centralPlanet,    // Planet to orbit around
        float orbitDistance,            // Distance from central planet
        float massRatio,               // Mass as ratio of central planet (0.0-1.0)
        float angleOffset = 0.0f,      // Starting angle in orbit (radians)
        sf::Color color = sf::Color::Green
    );

    // Calculate orbital velocity for circular orbit
    static float calculateOrbitalVelocity(float centralMass, float orbitDistance);

    // Calculate position on circular orbit
    static sf::Vector2f calculateOrbitalPosition(
        sf::Vector2f centerPos,
        float distance,
        float angle
    );
};