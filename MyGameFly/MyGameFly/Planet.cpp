#include "Planet.h"
#include "VectorHelper.h"
#include "GameConstants.h"
#include <cmath>
#include <iostream>  // For std::cout

Planet::Planet(sf::Vector2f pos, float radius, float mass, sf::Color color)
    : GameObject(pos, { 0, 0 }, color), mass(mass), planetID(-1), isNetworkPlanet(false),
    stateChanged(false), lastNetworkUpdateTime(0.0f), lastNetworkPosition(pos),
    lastNetworkVelocity({ 0, 0 }), lastNetworkMass(mass), networkSyncInterval(1.0f)
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

    // Update network synchronization timer
    lastNetworkUpdateTime += deltaTime;

    // Request network sync if enough time has passed and this isn't a network planet
    if (!isNetworkPlanet && lastNetworkUpdateTime >= networkSyncInterval && stateChanged) {
        // The SatelliteManager or NetworkManager will handle the actual sending
        lastNetworkUpdateTime = 0.0f;
    }
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
    if (std::abs(mass - newMass) > 0.01f) {  // Only update if change is significant
        mass = newMass;
        updateRadiusFromMass();

        // Trigger network synchronization for non-network planets
        if (!isNetworkPlanet) {
            requestStateSync();
        }
    }
}

void Planet::setRadius(float newRadius) {
    if (std::abs(radius - newRadius) > 0.01f) {  // Only update if change is significant
        radius = newRadius;

        // Update the visual shape
        shape.setRadius(radius);
        shape.setOrigin({ radius, radius });

        // Trigger network synchronization for non-network planets
        if (!isNetworkPlanet) {
            requestStateSync();
        }
    }
}

void Planet::setVelocity(sf::Vector2f vel) {
    if (distance(velocity, vel) > 0.01f) {  // Only update if change is significant
        velocity = vel;

        // Trigger network synchronization for non-network planets  
        if (!isNetworkPlanet) {
            requestStateSync();
        }
    }
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

    // Choose color based on collection status and network ownership
    sf::Color ringColor = isActivelyCollecting ?
    GameConstants::FUEL_RING_ACTIVE_COLOR :
    GameConstants::FUEL_RING_COLOR;

    // Modify color if this is a network planet to distinguish ownership
    if (isNetworkPlanet) {
        ringColor.a = 128;  // Make network planets' rings semi-transparent
    }

    // Create ring shape
    sf::CircleShape ring;
    ring.setRadius(ringRadius);
    ring.setPosition({ position.x - ringRadius, position.y - ringRadius });
    ring.setFillColor(sf::Color::Transparent);
    ring.setOutlineColor(ringColor);
    ring.setOutlineThickness(2.0f);

    window.draw(ring);

    // Draw network indicator if this is a network planet
    if (isNetworkPlanet) {
        drawNetworkIndicator(window);
    }
}

void Planet::drawNetworkIndicator(sf::RenderWindow& window) {
    if (!isNetworkPlanet || planetID < 0) return;

    // Draw a small indicator showing this is a network-synced planet
    float indicatorSize = radius * 0.1f;

    sf::CircleShape networkIndicator;
    networkIndicator.setRadius(indicatorSize);
    networkIndicator.setPosition(sf::Vector2f(
        position.x + radius - indicatorSize,
        position.y - radius - indicatorSize
    ));

    // Use a distinct color for network indicators
    networkIndicator.setFillColor(sf::Color(255, 255, 0, 180));  // Semi-transparent yellow
    networkIndicator.setOutlineColor(sf::Color::Black);
    networkIndicator.setOutlineThickness(1.0f);

    window.draw(networkIndicator);
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

// Network synchronization methods
void Planet::updateFromNetworkState(sf::Vector2f netPosition, sf::Vector2f netVelocity, float netMass, float netRadius) {
    if (!isNetworkPlanet) return;

    // Smooth interpolation for network updates to avoid jittery movement
    float interpolationFactor = 0.1f;  // Adjust for smoother/snappier updates

    position = position + (netPosition - position) * interpolationFactor;
    velocity = velocity + (netVelocity - velocity) * interpolationFactor;

    // Update mass and radius directly (more critical for gameplay)
    if (std::abs(mass - netMass) > 0.01f) {
        mass = netMass;
        updateRadiusFromMass();
    }

    if (std::abs(radius - netRadius) > 0.01f) {
        radius = netRadius;
        shape.setRadius(radius);
        shape.setOrigin({ radius, radius });
    }

    // Update shape position
    shape.setPosition(position);

    lastNetworkPosition = netPosition;
    lastNetworkVelocity = netVelocity;
    lastNetworkMass = netMass;
    lastNetworkUpdateTime = 0.0f;  // Reset update timer

    std::cout << "Updated planet " << planetID << " from network state (Mass: " << mass << ")" << std::endl;
}

struct PlanetState Planet::createPlanetState() const {
    struct PlanetState state;
    state.planetID = planetID;
    state.position = position;
    state.velocity = velocity;
    state.mass = mass;
    state.radius = radius;
    state.color = color;
    return state;
}

void Planet::applyPlanetState(const struct PlanetState& state) {
    planetID = state.planetID;

    // Don't interpolate if this is the first network update
    if (isNetworkPlanet) {
        updateFromNetworkState(state.position, state.velocity, state.mass, state.radius);
    }
    else {
        // Direct assignment for first-time network planets
        position = state.position;
        velocity = state.velocity;
        mass = state.mass;
        radius = state.radius;
        color = state.color;

        // Update visual shape
        shape.setRadius(radius);
        shape.setOrigin({ radius, radius });
        shape.setPosition(position);
        shape.setFillColor(color);

        isNetworkPlanet = true;
    }

    lastNetworkPosition = state.position;
    lastNetworkVelocity = state.velocity;
    lastNetworkMass = state.mass;
}
void Planet::restoreFromSaveState(const PlanetSaveState& saveState) {
    // Restore all planet properties from save state
    position = saveState.position;
    velocity = saveState.velocity;
    mass = saveState.mass;
    radius = saveState.radius;
    color = saveState.color;

    // Update visual representation
    shape.setRadius(radius);
    shape.setOrigin({ radius, radius });
    shape.setPosition(position);
    shape.setFillColor(color);

    // Reset network state tracking since this is a loaded state
    stateChanged = false;
    lastNetworkPosition = position;
    lastNetworkVelocity = velocity;
    lastNetworkMass = mass;
    lastNetworkUpdateTime = 0.0f;

    std::cout << "Planet restored from save state (Mass: " << mass << ", Position: "
        << position.x << ", " << position.y << ")" << std::endl;
}