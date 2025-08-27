#pragma once
#include "GameObject.h"
#include <vector>
#include <memory>

// Forward declaration for moon configuration
struct MoonConfig;

// Planet configuration structure for dynamic planet creation
// In Planet.h, update the PlanetConfig structure:
struct PlanetConfig {
    float massRatio;                    // Mass as ratio of main planet
    float orbitDistanceMultiplier;      // Distance multiplier from base orbit
    float angleOffset;                  // Starting angle offset (radians)
    sf::Color color;
    std::vector<MoonConfig> moons;      // Moon configurations for this planet

    PlanetConfig(float mass = 0.06f, float dist = 1.0f,
        float angle = 0.0f, sf::Color col = sf::Color::Green)
        : massRatio(mass), orbitDistanceMultiplier(dist),
        angleOffset(angle), color(col) {
    }
};
// Moon configuration structure
struct MoonConfig {
    float massRatio;                    // Mass as ratio of parent planet (0.01-0.1 typically)
    float orbitDistanceMultiplier;      // Distance from parent planet (in planet radii)
    float angleOffset;                  // Starting angle in orbit
    sf::Color color;

    MoonConfig(float mass = 0.02f, float dist = 3.0f,
        float angle = 0.0f, sf::Color col = sf::Color(200, 200, 200))
        : massRatio(mass), orbitDistanceMultiplier(dist),
        angleOffset(angle), color(col) {
    }
};

class Planet : public GameObject {
private:
    sf::CircleShape shape;
    float mass;
    float radius;

    // Moon system members
    Planet* parentPlanet;                           // nullptr if this is a primary planet
    std::vector<std::unique_ptr<Planet>> moons;     // Child moons orbiting this planet
    bool isMoon;                                    // Flag to identify moons
    float moonRetentionMassThreshold;               // Minimum mass to retain moons
    float sphereOfInfluenceRadius;                  // Gravitational dominance radius

    // Orbital tracking for moons
    float orbitalAngle;                             // Current angle in orbit (for moons)
    float orbitalDistance;                          // Distance from parent (for moons)
    float orbitalAngularVelocity;                   // Angular velocity around parent

    // Visual properties for moons
    float moonVisualScale;                          // Scale factor for moon rendering
    bool showOrbitPath;                             // Whether to show this object's orbit

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

    // MOON SYSTEM METHODS
    // Moon management
    void addMoon(std::unique_ptr<Planet> moon);
    void removeMoon(int moonIndex);
    void clearMoons();
    const std::vector<std::unique_ptr<Planet>>& getMoons() const { return moons; }
    std::vector<Planet*> getAllMoonPointers() const;

    // Moon physics
    void updateMoons(float deltaTime);
    void checkMoonRetention();                      // Check if moons should escape
    std::vector<std::unique_ptr<Planet>> releaseLostMoons();  // Release moons that escape

    // Hierarchical gravity
    void setParentPlanet(Planet* parent);
    Planet* getParentPlanet() const { return parentPlanet; }
    bool getIsMoon() const { return isMoon; }
    void setIsMoon(bool moonStatus) { isMoon = moonStatus; }

    // Sphere of influence
    float calculateSphereOfInfluence(float parentMass = 0.0f) const;
    bool isWithinSphereOfInfluence(sf::Vector2f point) const;
    float getSphereOfInfluenceRadius() const { return sphereOfInfluenceRadius; }
    void updateSphereOfInfluence();

    // Moon orbital mechanics
    void updateMoonOrbit(float deltaTime);          // Update position relative to parent
    sf::Vector2f getWorldPosition() const;          // Get absolute position (accounting for parent)
    sf::Vector2f getRelativePosition() const;       // Get position relative to parent
    void setOrbitalParameters(float distance, float angle, float angularVel);

    // Visual methods for moons
    void drawMoonOrbitPath(sf::RenderWindow& window);
    void drawSphereOfInfluence(sf::RenderWindow& window, sf::Color color = sf::Color(100, 100, 255, 50));
    void drawMoonConnectionLine(sf::RenderWindow& window);  // Line to parent planet
    void setMoonVisualScale(float scale) { moonVisualScale = scale; }

    // Mass threshold management
    void setMoonRetentionThreshold(float threshold) { moonRetentionMassThreshold = threshold; }
    float getMoonRetentionThreshold() const { return moonRetentionMassThreshold; }
    bool canRetainMoons() const { return mass >= moonRetentionMassThreshold; }

    // STATIC METHODS FOR DYNAMIC PLANET CREATION
    // Static method to create an orbiting planet
    static std::unique_ptr<Planet> createOrbitalPlanet(
        const Planet* centralPlanet,    // Planet to orbit around
        float orbitDistance,            // Distance from central planet
        float massRatio,               // Mass as ratio of central planet (0.0-1.0)
        float angleOffset = 0.0f,      // Starting angle in orbit (radians)
        sf::Color color = sf::Color::Green
    );

    // Static method to create a moon
    static std::unique_ptr<Planet> createMoon(
        Planet* parentPlanet,           // Planet this moon orbits
        float orbitDistance,            // Distance from parent (in parent radii)
        float massRatio,               // Mass as ratio of parent planet
        float angleOffset = 0.0f,      // Starting angle in orbit
        sf::Color color = sf::Color(200, 200, 200)  // Default moon color (gray)
    );

    // Create multiple moons based on planet mass
    static std::vector<std::unique_ptr<Planet>> createMoonsForPlanet(
        Planet* parentPlanet,
        const std::vector<MoonConfig>& moonConfigs
    );

    // Determine number of moons based on mass
    static int calculateMoonCount(float planetMass, float mainPlanetMass);

    // Calculate orbital velocity for circular orbit
    static float calculateOrbitalVelocity(float centralMass, float orbitDistance);

    // Calculate position on circular orbit
    static sf::Vector2f calculateOrbitalPosition(
        sf::Vector2f centerPos,
        float distance,
        float angle
    );

    // Calculate moon escape velocity
    static float calculateEscapeVelocity(float parentMass, float distance);

    // Check if moon should escape based on parent mass loss
    static bool shouldMoonEscape(float currentParentMass, float originalParentMass,
        float moonDistance, float escapeThreshold = 0.7f);
};