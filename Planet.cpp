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
    // Initialize moon system members
    parentPlanet = nullptr;
    isMoon = false;
    moonRetentionMassThreshold = mass * 1.0f;  // Lose moons if mass drops below 100%
    sphereOfInfluenceRadius = calculateSphereOfInfluence();
    orbitalAngle = 0.0f;
    orbitalDistance = 0.0f;
    orbitalAngularVelocity = 0.0f;
    moonVisualScale = 1.0f;
    showOrbitPath = true;
}

void Planet::update(float deltaTime)
{
    // Update moon orbit if this is a moon
    if (isMoon && parentPlanet) {
        updateMoonOrbit(deltaTime);
    }
    else {
        // Normal planet movement
        position += velocity * deltaTime;
    }

    shape.setPosition(position);

    // Update all child moons
    updateMoons(deltaTime);

    // Check if moons should escape (for planets with moons)
    if (!moons.empty()) {
        checkMoonRetention();
    }
}
void Planet::draw(sf::RenderWindow& window)
{
    // Draw the planet
    window.draw(shape);

    // Draw moons
    for (auto& moon : moons) {
        if (moon) {
            moon->draw(window);
        }
    }

    // Draw moon orbit path if this is a moon
    if (isMoon && showOrbitPath) {
        drawMoonOrbitPath(window);
    }

    // Draw sphere of influence for planets with moons
    if (!moons.empty()) {
        drawSphereOfInfluence(window);
    }

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
    // Update sphere of influence
    updateSphereOfInfluence();

    // Check if moons should be lost due to mass reduction
    if (!moons.empty()) {
        checkMoonRetention();
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

// STATIC METHODS FOR DYNAMIC PLANET CREATION

std::unique_ptr<Planet> Planet::createOrbitalPlanet(
    const Planet* centralPlanet,
    float orbitDistance,
    float massRatio,
    float angleOffset,
    sf::Color color)
{
    // Calculate mass
    float planetMass = centralPlanet->getMass() * massRatio;

    // Calculate orbital position
    sf::Vector2f centerPos = centralPlanet->getPosition();
    sf::Vector2f orbitPos = calculateOrbitalPosition(centerPos, orbitDistance, angleOffset);

    // Create the planet with 0 radius (will be calculated from mass)
    auto planet = std::make_unique<Planet>(orbitPos, 0.0f, planetMass, color);

    // Calculate and set orbital velocity
    float orbitalVel = calculateOrbitalVelocity(centralPlanet->getMass(), orbitDistance);

    // Velocity is perpendicular to radius vector
    // For counter-clockwise orbit: rotate radius vector 90 degrees
    sf::Vector2f velocityDirection(-std::sin(angleOffset), std::cos(angleOffset));
    planet->setVelocity(velocityDirection * orbitalVel);

    return planet;
}

float Planet::calculateOrbitalVelocity(float centralMass, float orbitDistance)
{
    return std::sqrt(GameConstants::G * centralMass / orbitDistance);
}

sf::Vector2f Planet::calculateOrbitalPosition(sf::Vector2f centerPos, float distance, float angle)
{
    return sf::Vector2f(
        centerPos.x + distance * std::cos(angle),
        centerPos.y + distance * std::sin(angle)
    );
}

// Moon system implementation
void Planet::addMoon(std::unique_ptr<Planet> moon) {
    if (moon) {
        moon->setParentPlanet(this);
        moon->setIsMoon(true);
        moons.push_back(std::move(moon));
    }
}

void Planet::updateMoons(float deltaTime) {
    for (auto& moon : moons) {
        if (moon) {
            moon->update(deltaTime);
        }
    }
}

void Planet::updateMoonOrbit(float deltaTime) {
    if (!parentPlanet) return;

    // Update orbital angle
    orbitalAngle += orbitalAngularVelocity * deltaTime;

    // Calculate new position relative to parent
    sf::Vector2f parentPos = parentPlanet->getPosition();
    position.x = parentPos.x + orbitalDistance * std::cos(orbitalAngle);
    position.y = parentPos.y + orbitalDistance * std::sin(orbitalAngle);

    // Update velocity to match orbital motion (for physics interactions)
    velocity.x = -orbitalDistance * orbitalAngularVelocity * std::sin(orbitalAngle);
    velocity.y = orbitalDistance * orbitalAngularVelocity * std::cos(orbitalAngle);

    // Add parent's velocity for correct world-space velocity
    velocity = velocity + parentPlanet->getVelocity();
}

void Planet::checkMoonRetention() {
    if (mass < moonRetentionMassThreshold) {
        // Moons will be released in the next update
        for (auto& moon : moons) {
            if (moon) {
                moon->setParentPlanet(nullptr);
                moon->setIsMoon(false);
                // Add escape velocity perturbation
                float escapeSpeed = calculateEscapeVelocity(mass, moon->orbitalDistance);
                sf::Vector2f escapeDir = normalize(moon->getPosition() - position);
                moon->setVelocity(moon->getVelocity() + escapeDir * escapeSpeed * 0.0f);
            }
        }
    }
}

void Planet::setParentPlanet(Planet* parent) {
    parentPlanet = parent;
    if (parent) {
        sphereOfInfluenceRadius = calculateSphereOfInfluence(parent->getMass());
    }
}

float Planet::calculateSphereOfInfluence(float parentMass) const {
    if (parentMass <= 0) {
        return radius * 10.0f;  // Default for primary planets
    }
    // Only calculate distance if we have a parent
    if (!parentPlanet) {
        return radius * 10.0f;
    }
    // Hill sphere approximation
    float distance = std::sqrt((position.x - parentPlanet->position.x) * (position.x - parentPlanet->position.x) +
        (position.y - parentPlanet->position.y) * (position.y - parentPlanet->position.y));
    return distance * std::pow(mass / (3.0f * parentMass), 1.0f / 3.0f);
}
void Planet::updateSphereOfInfluence() {
    if (parentPlanet) {
        sphereOfInfluenceRadius = calculateSphereOfInfluence(parentPlanet->getMass());
    }
    else {
        sphereOfInfluenceRadius = calculateSphereOfInfluence();
    }
}

void Planet::setOrbitalParameters(float distance, float angle, float angularVel) {
    orbitalDistance = distance;
    orbitalAngle = angle;
    orbitalAngularVelocity = angularVel;
}

void Planet::drawMoonOrbitPath(sf::RenderWindow& window) {
    if (!parentPlanet) return;

    sf::CircleShape orbit;
    orbit.setRadius(orbitalDistance);
    orbit.setPosition(parentPlanet->getPosition() - sf::Vector2f(orbitalDistance, orbitalDistance));
    orbit.setFillColor(sf::Color::Transparent);
    orbit.setOutlineColor(sf::Color(150, 150, 150, 100));
    orbit.setOutlineThickness(1.0f);
    orbit.setPointCount(60);
    window.draw(orbit);
}

void Planet::drawSphereOfInfluence(sf::RenderWindow& window, sf::Color color) {
    sf::CircleShape influence;
    influence.setRadius(sphereOfInfluenceRadius);
    influence.setPosition(position - sf::Vector2f(sphereOfInfluenceRadius, sphereOfInfluenceRadius));
    influence.setFillColor(sf::Color::Transparent);
    influence.setOutlineColor(color);
    influence.setOutlineThickness(1.0f);
    influence.setPointCount(40);
    window.draw(influence);
}

std::vector<std::unique_ptr<Planet>> Planet::releaseLostMoons() {
    std::vector<std::unique_ptr<Planet>> lostMoons;
    if (mass >= moonRetentionMassThreshold) return lostMoons;

    for (auto& moon : moons) {
        if (moon) {
            lostMoons.push_back(std::move(moon));
        }
    }
    moons.clear();
    return lostMoons;
}

sf::Vector2f Planet::getWorldPosition() const {
    if (isMoon && parentPlanet) {
        return position;  // Already calculated in world space during updateMoonOrbit
    }
    return position;
}

// Static moon creation methods
std::unique_ptr<Planet> Planet::createMoon(Planet* parentPlanet, float orbitDistance,
    float massRatio, float angleOffset, sf::Color color) {
    if (!parentPlanet) return nullptr;

    float moonMass = parentPlanet->getMass() * massRatio;
    float actualDistance = parentPlanet->getRadius() * orbitDistance;

    sf::Vector2f moonPos = calculateOrbitalPosition(parentPlanet->getPosition(), actualDistance, angleOffset);

    auto moon = std::make_unique<Planet>(moonPos, 0.0f, moonMass, color);
    moon->setIsMoon(true);
    moon->setParentPlanet(parentPlanet);

    // Set orbital parameters
    float orbitalVel = calculateOrbitalVelocity(parentPlanet->getMass(), actualDistance);
    float angularVel = orbitalVel / actualDistance;
    moon->setOrbitalParameters(actualDistance, angleOffset, angularVel);

    // Set moon visual scale
    moon->setMoonVisualScale(0.5f);  // Moons appear smaller

    return moon;
}

int Planet::calculateMoonCount(float planetMass, float mainPlanetMass) {
    float massRatio = planetMass / mainPlanetMass;
    if (massRatio < 0.02f) return 0;      // Too small for moons
    if (massRatio < 0.05f) return 1;      // Small planet - 1 moon
    if (massRatio < 0.10f) return 2;      // Medium planet - 2 moons
    return 3;                             // Large planet - 3 moons max
}

float Planet::calculateEscapeVelocity(float parentMass, float distance) {
    return std::sqrt(2.0f * GameConstants::G * parentMass / distance);
}

// Add these methods to Planet.cpp:

void Planet::removeMoon(int moonIndex) {
    if (moonIndex >= 0 && moonIndex < moons.size()) {
        moons.erase(moons.begin() + moonIndex);
    }
}

void Planet::clearMoons() {
    moons.clear();
}

std::vector<Planet*> Planet::getAllMoonPointers() const {
    std::vector<Planet*> moonPtrs;
    for (const auto& moon : moons) {
        if (moon) {
            moonPtrs.push_back(moon.get());
        }
    }
    return moonPtrs;
}

bool Planet::isWithinSphereOfInfluence(sf::Vector2f point) const {
    float distance = std::sqrt((point.x - position.x) * (point.x - position.x) +
        (point.y - position.y) * (point.y - position.y));
    return distance <= sphereOfInfluenceRadius;
}

sf::Vector2f Planet::getRelativePosition() const {
    if (isMoon && parentPlanet) {
        return position - parentPlanet->getPosition();
    }
    return position;
}

void Planet::drawMoonConnectionLine(sf::RenderWindow& window) {
    if (!isMoon || !parentPlanet) return;

    sf::VertexArray line(sf::PrimitiveType::LineStrip);

    sf::Vertex start;
    start.position = position;
    start.color = sf::Color(100, 100, 100, 50);  // Faint gray
    line.append(start);

    sf::Vertex end;
    end.position = parentPlanet->getPosition();
    end.color = sf::Color(100, 100, 100, 50);
    line.append(end);

    window.draw(line);
}

// Static method for creating multiple moons
std::vector<std::unique_ptr<Planet>> Planet::createMoonsForPlanet(
    Planet* parentPlanet, const std::vector<MoonConfig>& moonConfigs) {

    std::vector<std::unique_ptr<Planet>> newMoons;

    for (const auto& config : moonConfigs) {
        auto moon = createMoon(parentPlanet,
            config.orbitDistanceMultiplier,
            config.massRatio,
            config.angleOffset,
            config.color);
        if (moon) {
            newMoons.push_back(std::move(moon));
        }
    }

    return newMoons;
}

// Static method to check if moon should escape
bool Planet::shouldMoonEscape(float currentParentMass, float originalParentMass,
    float moonDistance, float escapeThreshold) {
    float massRatio = currentParentMass / originalParentMass;
    return massRatio < escapeThreshold;
}