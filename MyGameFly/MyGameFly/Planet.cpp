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

void Planet::setPosition(sf::Vector2f pos)
{
    position = pos;
    shape.setPosition(position);
}

void Planet::draw(sf::RenderWindow& window)
{
    // Draw the planet
    window.draw(shape);

    // Draw fuel collection ring if the planet can provide fuel
    if (canCollectFuel()) {
        drawFuelCollectionRing(window, false);
    }
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

bool Planet::canCollectFuel() const
{
    return mass > GameConstants::MIN_PLANET_MASS_FOR_COLLECTION;
}

float Planet::getFuelCollectionRange() const
{
    return radius + GameConstants::FUEL_COLLECTION_RANGE;
}

void Planet::drawFuelCollectionRing(sf::RenderWindow& window, bool isActivelyCollecting)
{
    if (!canCollectFuel()) {
        return;  // Don't draw ring if planet can't provide fuel
    }

    // Calculate ring radius
    float ringRadius = getFuelCollectionRange();

    // Choose color based on collection status
    sf::Color ringColor = isActivelyCollecting ?
        GameConstants::FUEL_RING_ACTIVE_COLOR :
        GameConstants::FUEL_RING_COLOR;

    // Create ring using CircleShape with transparent fill
    sf::CircleShape ring;
    ring.setRadius(ringRadius);
    ring.setPosition(sf::Vector2f(position.x - ringRadius, position.y - ringRadius));
    ring.setFillColor(sf::Color::Transparent);
    ring.setOutlineColor(ringColor);
    ring.setOutlineThickness(GameConstants::FUEL_RING_THICKNESS);

    // Draw the ring
    window.draw(ring);

    // For active collection, add a pulsing inner ring effect
    if (isActivelyCollecting) {
        // Create a smaller, more opaque inner ring
        float innerRingRadius = ringRadius * 0.8f;
        sf::CircleShape innerRing;
        innerRing.setRadius(innerRingRadius);
        innerRing.setPosition(sf::Vector2f(position.x - innerRingRadius, position.y - innerRingRadius));
        innerRing.setFillColor(sf::Color::Transparent);

        // Make the inner ring more vibrant
        sf::Color innerRingColor = GameConstants::FUEL_RING_ACTIVE_COLOR;
        innerRingColor.a = 255;  // Full opacity
        innerRing.setOutlineColor(innerRingColor);
        innerRing.setOutlineThickness(GameConstants::FUEL_RING_THICKNESS * 0.5f);

        window.draw(innerRing);
    }
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