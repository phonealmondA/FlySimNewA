#pragma once
#include "GameObject.h"
#include <vector>
#include "NetworkManager.h"  // For PlanetState definition

struct PlanetSaveState;  // Forward declaration for save state support

class Planet : public GameObject {
private:
    sf::CircleShape shape;
    float mass;
    float radius;

    // Network synchronization support - NEW
    int planetID;                        // Unique identifier for network sync
    bool isNetworkPlanet;               // Created/updated by network
    bool stateChanged;                  // State needs network sync
    float lastNetworkUpdateTime;        // Time since last network update
    sf::Vector2f lastNetworkPosition;   // Last position from network
    sf::Vector2f lastNetworkVelocity;   // Last velocity from network
    float lastNetworkMass;              // Last mass from network
    float networkSyncInterval;          // How often to sync with network

public:
    Planet(sf::Vector2f pos, float radius, float mass, sf::Color color = sf::Color::Blue);

    void update(float deltaTime) override;
    void draw(sf::RenderWindow& window) override;
    void setPosition(sf::Vector2f pos) override;

    float getMass() const;
    float getRadius() const;

    // New methods for dynamic radius
    void setMass(float newMass);
    void updateRadiusFromMass();

    // Draw velocity vector for the planet
    void drawVelocityVector(sf::RenderWindow& window, float scale = 1.0f);

    // Draw predicted orbit path
    void drawOrbitPath(sf::RenderWindow& window, const std::vector<Planet*>& planets,
        float timeStep = 0.5f, int steps = 200);
    // FUEL COLLECTION METHODS
    void drawFuelCollectionRing(sf::RenderWindow& window, bool isActivelyCollecting = false);
    bool canCollectFuel() const;  // Check if planet has enough mass for fuel collection
    float getFuelCollectionRange() const;  // Get the fuel collection range for this planet

    // Network synchronization support - NEW
    void setColor(sf::Color newColor) { color = newColor; shape.setFillColor(newColor); }
    sf::Color getColor() const { return color; }
    void setPlanetID(int id) { planetID = id; }
    int getPlanetID() const { return planetID; }

    // Network state management
    void updateFromNetworkState(sf::Vector2f netPosition, sf::Vector2f netVelocity, float netMass, float netRadius);
    void setNetworkPlanet(bool isNetwork) { isNetworkPlanet = isNetwork; }
    bool isFromNetwork() const { return isNetworkPlanet; }

    // Visual network indicator
    void drawNetworkIndicator(sf::RenderWindow& window);

    // Network synchronization helpers
    bool hasStateChanged() const { return stateChanged; }
    void markStateSynced() { stateChanged = false; }
    void requestStateSync() { stateChanged = true; }

    // Enhanced setters that trigger network sync
    void setVelocity(sf::Vector2f vel);
    void setRadius(float newRadius);

    // Planet state conversion for network
    struct PlanetState createPlanetState() const;
    void applyPlanetState(const struct PlanetState& state);
    // Save state management
    void restoreFromSaveState(const struct PlanetSaveState& saveState);
};