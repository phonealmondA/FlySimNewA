#include "Planet.h"
#include "VectorHelper.h"
#include "GameConstants.h"
#include <cmath>

Planet::Planet(sf::Vector2f pos, float radius, float mass, sf::Color color)
    : GameObject(pos, { 0, 0 }, color), mass(mass)
{
    // If a specific radius was provided, use it
    if (radius > 0) {
        this->radius = radius;
    }
    else {
        // Otherwise calculate from mass
        updateRadiusFromMass();
    }

    shape.setRadius(this->radius);
    shape.setFillColor(color);
    shape.setOrigin({ this->radius, this->radius });
    shape.setPosition(position);
}

void Planet::update(float deltaTime)
{
    position += velocity * deltaTime;
    shape.setPosition(position);
}

void Planet::draw(sf::RenderWindow& window)
{
    window.draw(shape);
}

float Planet::getMass() const
{
    return mass;
}

float Planet::getRadius() const
{
    return radius;
}

void Planet::setMass(float newMass)
{
    mass = newMass;
    updateRadiusFromMass();
}

void Planet::updateRadiusFromMass()
{
    // Use cube root relationship between mass and radius
    radius = GameConstants::BASE_RADIUS_FACTOR *
        std::pow(mass / GameConstants::REFERENCE_MASS, 1.0f / 3.0f);

    // Update the visual shape
    shape.setRadius(radius);
    shape.setOrigin({ radius, radius });
}

void Planet::drawVelocityVector(sf::RenderWindow& window, float scale)
{
    sf::VertexArray line(sf::PrimitiveType::LineStrip);

    sf::Vertex startVertex;
    startVertex.position = position;
    startVertex.color = sf::Color::Yellow;
    line.append(startVertex);

    sf::Vertex endVertex;
    endVertex.position = position + velocity * scale;
    endVertex.color = sf::Color::Green;
    line.append(endVertex);

    window.draw(line);
}

void Planet::drawOrbitPath(sf::RenderWindow& window, const std::vector<Planet*>& planets,
    float timeStep, int steps)
{
    // Create a vertex array for the trajectory line
    sf::VertexArray trajectory(sf::PrimitiveType::LineStrip);

    // Start with current position and velocity
    sf::Vector2f simPosition = position;
    sf::Vector2f simVelocity = velocity;

    // Add the starting point
    sf::Vertex startPoint;
    startPoint.position = simPosition;
    startPoint.color = sf::Color(color.r, color.g, color.b, 100); // Semi-transparent version of planet color
    trajectory.append(startPoint);

    // Simulate future positions
    for (int i = 0; i < steps; i++) {
        // Calculate gravitational forces from all planets
        sf::Vector2f totalAcceleration(0, 0);

        for (const auto& otherPlanet : planets) {
            // Skip self
            if (otherPlanet == this) continue;

            sf::Vector2f direction = otherPlanet->getPosition() - simPosition;
            float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

            // Skip if too close
            if (distance <= otherPlanet->getRadius() + radius + GameConstants::TRAJECTORY_COLLISION_RADIUS) {
                // Stop the trajectory if we hit another planet
                break;
            }

            // Use same gravitational constant as in GravitySimulator
            const float G = GameConstants::G;  // Use the constant from the header
            float forceMagnitude = G * otherPlanet->getMass() * mass / (distance * distance);

            sf::Vector2f acceleration = normalize(direction) * forceMagnitude / mass;
            totalAcceleration += acceleration;
        }

        // Update simulated velocity and position
        simVelocity += totalAcceleration * timeStep;
        simPosition += simVelocity * timeStep;

        // Calculate fade-out effect
        float alpha = 255 * (1.0f - static_cast<float>(i) / steps);
        // Use uint8_t instead of sf::Uint8
        sf::Color pointColor(color.r, color.g, color.b, static_cast<uint8_t>(alpha));

        // Add point to trajectory
        sf::Vertex point;
        point.position = simPosition;
        point.color = pointColor;
        trajectory.append(point);
    }

    // Draw the trajectory
    window.draw(trajectory);
}