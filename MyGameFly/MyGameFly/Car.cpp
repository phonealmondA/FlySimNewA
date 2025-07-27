// Car.cpp
#include "Car.h"
#include "Rocket.h"
#include "VectorHelper.h"
#include "GameConstants.h"
#include <cmath>
#include <float.h>

Car::Car(sf::Vector2f pos, sf::Vector2f vel, sf::Color col)
    : GameObject(pos, vel, col), rotation(0), speed(0), maxSpeed(200.0f),
    currentPlanet(nullptr), isGrounded(false)
{
    // Create car body (small box)
    body.setSize({ GameConstants::CAR_BODY_WIDTH, GameConstants::CAR_BODY_HEIGHT });
    body.setFillColor(color);
    body.setOrigin({ GameConstants::CAR_BODY_WIDTH / 2, GameConstants::CAR_BODY_HEIGHT / 2 });
    body.setPosition(position);

    // Create wheels
    for (int i = 0; i < 2; i++) {
        wheels[i].setRadius(GameConstants::CAR_WHEEL_RADIUS);
        wheels[i].setFillColor(sf::Color::Black);
        wheels[i].setOrigin({ 5.0f, 5.0f });
    }

    // Create direction arrow
    directionArrow.setPointCount(3);
    directionArrow.setPoint(0, { 10.0f, 0.0f });
    directionArrow.setPoint(1, { 0.0f, -5.0f });
    directionArrow.setPoint(2, { 0.0f, 5.0f });
    directionArrow.setFillColor(sf::Color::Red);
    directionArrow.setOrigin({ 0.0f, 0.0f });
}

void Car::accelerate(float amount) {
    if (isGrounded) {
        speed += amount * 10.0f; // Increased for better response
        speed = std::min(speed, maxSpeed);
        speed = std::max(speed, -maxSpeed / 2.0f); // Slower in reverse
    }
}

void Car::rotate(float amount) {
    if (isGrounded) {
        rotation += amount * 2.0f; // Increased for better response
    }
}

void Car::checkGrounding(const std::vector<Planet*>& planets) {
    isGrounded = false;
    float closestDistance = FLT_MAX;
    currentPlanet = nullptr;

    for (const auto& planet : planets) {
        sf::Vector2f direction = position - planet->getPosition();
        float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

        // Check if we're at or very close to the surface
        if (distance <= (planet->getRadius() + GameConstants::ROCKET_SIZE) && distance < closestDistance) {
            closestDistance = distance;
            currentPlanet = const_cast<Planet*>(planet);
            isGrounded = true;
        }
    }
}

void Car::update(float deltaTime) {
    if (isGrounded && currentPlanet) {
        // Get direction to planet center
        sf::Vector2f toPlanet = currentPlanet->getPosition() - position;
        float distToPlanet = std::sqrt(toPlanet.x * toPlanet.x + toPlanet.y * toPlanet.y);
        sf::Vector2f normal = toPlanet / distToPlanet;

        // Calculate tangent direction (perpendicular to normal)
        sf::Vector2f tangent(-normal.y, normal.x);

        // Apply rotation
        float radians = rotation * 3.14159f / 180.0f;
        sf::Vector2f moveDir(std::cos(radians), std::sin(radians));

        // Project movement direction onto surface tangent
        float dotProduct = moveDir.x * tangent.x + moveDir.y * tangent.y;
        sf::Vector2f effectiveDir = tangent * dotProduct;

        // Move along surface
        position += effectiveDir * speed * deltaTime;

        // Apply gravity to stick to surface
        position = currentPlanet->getPosition() + normal * (currentPlanet->getRadius() + GameConstants::TRAJECTORY_COLLISION_RADIUS);

        // Apply friction
        speed *= 0.98f;
    }
    else {
        // If in air, apply simple physics (fall with gravity)
        position += velocity * deltaTime;
    }

    // Update visual components
    body.setPosition(position);
    body.setRotation(sf::degrees(rotation));

    // Update wheels position
    float radians = rotation * 3.14159f / 180.0f;
    float cos_val = std::cos(radians);
    float sin_val = std::sin(radians);

    wheels[0].setPosition(position + sf::Vector2f(-10.0f * cos_val + 7.5f * sin_val,
        -10.0f * sin_val - 7.5f * cos_val));
    wheels[1].setPosition(position + sf::Vector2f(10.0f * cos_val + 7.5f * sin_val,
        10.0f * sin_val - 7.5f * cos_val));

    // Update arrow position
    directionArrow.setPosition(position + sf::Vector2f(15.0f * cos_val, 15.0f * sin_val));
    directionArrow.setRotation(sf::degrees(rotation));
}

void Car::draw(sf::RenderWindow& window) {
    window.draw(body);
    window.draw(wheels[0]);
    window.draw(wheels[1]);
    window.draw(directionArrow);
}

void Car::drawWithConstantSize(sf::RenderWindow& window, float zoomLevel) {
    // Simply call the regular draw for now
    draw(window);
}

void Car::initializeFromRocket(const Rocket* rocket) {
    position = rocket->getPosition();
    velocity = rocket->getVelocity() * GameConstants::TRANSFORM_VELOCITY_FACTOR;
    // You'll need to add this constant // Reduce velocity when transforming

    // Set car tangent to planet surface if near a planet
    if (currentPlanet) {
        sf::Vector2f toPlanet = currentPlanet->getPosition() - position;
        float distToPlanet = std::sqrt(toPlanet.x * toPlanet.x + toPlanet.y * toPlanet.y);
        sf::Vector2f normal = toPlanet / distToPlanet;

        // Calculate angle for proper orientation on the surface
        rotation = std::atan2(-normal.x, normal.y) * 180.0f / 3.14159f;
    }
}