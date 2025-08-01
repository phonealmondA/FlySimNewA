#pragma once
#include "GameObject.h"
#include <vector>

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
};