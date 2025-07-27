// Car.h
#pragma once
#include "GameObject.h"
#include "Planet.h"
#include <vector>

class Rocket; // Forward declaration

class Car : public GameObject {
private:
    sf::RectangleShape body;
    sf::CircleShape wheels[2];
    sf::ConvexShape directionArrow;
    float rotation;
    float speed;
    float maxSpeed;
    Planet* currentPlanet;
    bool isGrounded;

public:
    Car(sf::Vector2f pos, sf::Vector2f vel, sf::Color col = sf::Color::Green);

    void accelerate(float amount);
    void rotate(float amount);
    bool isOnGround() const { return isGrounded; }
    void checkGrounding(const std::vector<Planet*>& planets);

    void update(float deltaTime) override;
    void draw(sf::RenderWindow& window) override;
    void drawWithConstantSize(sf::RenderWindow& window, float zoomLevel);

    // Transfer state from rocket
    void initializeFromRocket(const Rocket* rocket);
    float getRotation() const { return rotation; }
};